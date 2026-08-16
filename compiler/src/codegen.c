#include "codegen.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// ============================================================
// HELPER FUNCTIONS FOR LITERAL DETECTION
// ============================================================

/**
 * Check if a string is a string literal (enclosed in double quotes)
 */
static int is_string_literal(const char *s)
{
  return s && s[0] == '"';
}

/**
 * Check if a string is a boolean literal (true or false)
 */
static int is_boolean_literal(const char *s)
{
  return s && (!strcmp(s, "true") || !strcmp(s, "false"));
}

/**
 * Check if a string is a decimal literal (contains a decimal point)
 */
static int is_decimal_literal(const char *s)
{
  if (!s || !*s)
  {
    return 0;
  }

  // String and boolean literals are not decimal
  if (is_string_literal(s) || is_boolean_literal(s))
  {
    return 0;
  }

  // Check for decimal point
  for (const char *p = s; *p; p++)
  {
    if (*p == '.')
    {
      return 1;
    }
  }
  return 0;
}

/**
 * Check if a string is an integer literal (digits only, optional minus sign)
 */
static int is_integer_literal(const char *s)
{
  if (!s || !*s)
  {
    return 0;
  }

  // String and boolean literals are not integers
  if (is_string_literal(s) || is_boolean_literal(s))
  {
    return 0;
  }

  // Skip optional minus sign
  int i = (*s == '-') ? 1 : 0;

  // All characters must be digits
  for (; s[i]; i++)
  {
    if (!isdigit(s[i]) && s[i] != '.')
    {
      return 0;
    }
  }

  // Must not contain a decimal point
  for (const char *p = s; *p; p++)
  {
    if (*p == '.')
    {
      return 0;
    }
  }

  return 1;
}

// ============================================================
// VARIABLE TRACKING FOR TYPE INFERENCE
// ============================================================

typedef struct
{
  char name[256]; // Variable name
  char type[32];  // Variable type (int, double, char*, etc.)
  int used;       // Flag indicating if variable is used
} VarInfo;

static VarInfo vars[512]; // Symbol table for inferred types
static int vcount = 0;    // Number of tracked variables

/**
 * Track a variable with its inferred type
 * Only adds if variable hasn't been tracked yet
 */
static void track_var(const char *name, const char *type)
{
  // Check if variable already exists
  for (int i = 0; i < vcount; i++)
  {
    if (strcmp(vars[i].name, name) == 0)
    {
      return; // Already tracked, skip
    }
  }

  // Add new variable if space available
  if (vcount < 512)
  {
    strncpy(vars[vcount].name, name, 255);
    vars[vcount].name[255] = '\0';

    strncpy(vars[vcount].type, type, 31);
    vars[vcount].type[31] = '\0';

    vars[vcount].used = 1;
    vcount++;
  }
}

// ============================================================
// TYPE INFERENCE FROM TAC INSTRUCTIONS
// ============================================================

/**
 * Infer variable types by analyzing TAC instructions
 * Scans through all TAC instructions and tracks variable types
 */
static void infer_types_from_tac(TACProgram *tac)
{
  for (int i = 0; i < tac->count; i++)
  {
    TACInstr *ins = &tac->instructions[i];

    // Skip control flow instructions
    if (ins->op == TAC_LABEL ||
        ins->op == TAC_JUMP ||
        ins->op == TAC_JUMP_IF_FALSE ||
        ins->op == TAC_OUTPUT ||
        ins->op == TAC_TASK_CALL ||
        ins->op == TAC_RETURN)
    {
      continue;
    }

    const char *name = ins->result;
    if (!name[0])
    {
      continue; // No result variable
    }

    // Temporary variables and return value register
    if (name[0] == 't' || !strcmp(name, "__rc"))
    {
      track_var(name, "double");
      continue;
    }

    // Determine type based on operation
    if (ins->op == TAC_ASSIGN)
    {
      // Assignment: infer type from RHS
      if (is_string_literal(ins->arg1))
      {
        track_var(name, "char*");
      }
      else if (is_boolean_literal(ins->arg1))
      {
        track_var(name, "int");
      }
      else if (is_decimal_literal(ins->arg1))
      {
        track_var(name, "double");
      }
      else if (is_integer_literal(ins->arg1))
      {
        track_var(name, "int");
      }
      else if (isalpha(ins->arg1[0]) || ins->arg1[0] == '_')
      {
        // Copy type from source variable
        for (int j = 0; j < vcount; j++)
        {
          if (strcmp(vars[j].name, ins->arg1) == 0)
          {
            track_var(name, vars[j].type);
            break;
          }
        }
      }
      else
      {
        track_var(name, "double"); // Default to double
      }
    }
    else if (ins->op == TAC_INPUT)
    {
      track_var(name, "char*"); // Input returns string
    }
    else
    {
      track_var(name, "double"); // Arithmetic ops return double
    }
  }
}

// ============================================================
// CODE GENERATION HELPERS
// ============================================================

/**
 * Append a string to the generated code buffer
 */
static void append_code(CodeGenResult *r, const char *s)
{
  int n = strlen(s);

  // Check buffer overflow
  if (r->length + n >= 65535)
  {
    return;
  }

  memcpy(r->code + r->length, s, n);
  r->length += n;
  r->code[r->length] = '\0';
}

// ============================================================
// MAIN CODE GENERATION FUNCTION
// ============================================================

/**
 * Generate C code from TAC (Three Address Code)
 * Performs type inference and produces runnable C program
 */
CodeGenResult codegen_generate(TACProgram *tac)
{
  CodeGenResult result;
  result.length = 0;
  result.success = 1;
  result.error[0] = '\0';
  result.code[0] = '\0';

  // Reset variable tracking
  vcount = 0;

  // First pass: infer variable types
  infer_types_from_tac(tac);

  // ============================================================
  // GENERATE HEADER INCLUDES
  // ============================================================
  append_code(&result, "#include <stdio.h>\n");
  append_code(&result, "#include <stdlib.h>\n");
  append_code(&result, "#include <string.h>\n");
  append_code(&result, "#include <stdbool.h>\n\n");
  append_code(&result, "int main() {\n");

  // ============================================================
  // GENERATE VARIABLE DECLARATIONS
  // ============================================================
  for (int i = 0; i < vcount; i++)
  {
    char line[512];

    if (strcmp(vars[i].type, "char*") == 0)
    {
      // String variables: char array
      sprintf(line, "    char %s[4096];\n", vars[i].name);
    }
    else if (strcmp(vars[i].type, "double") == 0)
    {
      // Double variables
      sprintf(line, "    double %s = 0;\n", vars[i].name);
    }
    else
    {
      // Integer variables (default)
      sprintf(line, "    int %s = 0;\n", vars[i].name);
    }
    append_code(&result, line);
  }

  // ============================================================
  // GENERATE CODE FROM TAC INSTRUCTIONS
  // ============================================================
  char line[2048];

  for (int i = 0; i < tac->count; i++)
  {
    TACInstr *ins = &tac->instructions[i];

    switch (ins->op)
    {
    // ============================================================
    // CONTROL FLOW
    // ============================================================
    case TAC_LABEL:
      sprintf(line, "  %s:\n", ins->result);
      append_code(&result, line);
      break;

    case TAC_JUMP:
      sprintf(line, "    goto %s;\n", ins->result);
      append_code(&result, line);
      break;

    case TAC_JUMP_IF_FALSE:
      sprintf(line, "    if (!(%s)) goto %s;\n", ins->arg1, ins->result);
      append_code(&result, line);
      break;

    // ============================================================
    // ASSIGNMENT
    // ============================================================
    case TAC_ASSIGN:
    {
      const char *target = ins->result;
      char value[1024];

      // String assignment: use strcpy
      if (is_string_literal(ins->arg1) &&
          (target[0] != 't' && strcmp(target, "__rc")))
      {
        sprintf(line, "    strcpy(%s, %s);\n", target, ins->arg1);
      }
      // Boolean assignment: convert to integer
      else if (is_boolean_literal(ins->arg1))
      {
        const char *bool_val = (!strcmp(ins->arg1, "true")) ? "1" : "0";
        sprintf(line, "    %s = %s;\n", target, bool_val);
      }
      // Regular assignment
      else
      {
        sprintf(line, "    %s = %s;\n", target, ins->arg1);
      }
      append_code(&result, line);
      break;
    }

    // ============================================================
    // ARITHMETIC AND LOGICAL OPERATIONS
    // ============================================================
    case TAC_ADD:
    case TAC_SUB:
    case TAC_MUL:
    case TAC_DIV:
    case TAC_MOD:
    case TAC_EQ:
    case TAC_NEQ:
    case TAC_LT:
    case TAC_GT:
    case TAC_LTE:
    case TAC_GTE:
    case TAC_AND:
    case TAC_OR:
    {
      // Map TAC op to C operator
      const char *op = "+";
      switch (ins->op)
      {
      case TAC_ADD:
        op = "+";
        break;
      case TAC_SUB:
        op = "-";
        break;
      case TAC_MUL:
        op = "*";
        break;
      case TAC_DIV:
        op = "/";
        break;
      case TAC_MOD:
        op = "%";
        break;
      case TAC_EQ:
        op = "==";
        break;
      case TAC_NEQ:
        op = "!=";
        break;
      case TAC_LT:
        op = "<";
        break;
      case TAC_GT:
        op = ">";
        break;
      case TAC_LTE:
        op = "<=";
        break;
      case TAC_GTE:
        op = ">=";
        break;
      case TAC_AND:
        op = "&&";
        break;
      case TAC_OR:
        op = "||";
        break;
      default:
        op = "+";
        break;
      }

      // Convert boolean literals to integer values
      const char *a1 = ins->arg1;
      const char *a2 = ins->arg2;
      const char *a1_converted = is_boolean_literal(a1) ? (!strcmp(a1, "true") ? "1" : "0") : a1;
      const char *a2_converted = is_boolean_literal(a2) ? (!strcmp(a2, "true") ? "1" : "0") : a2;

      sprintf(line, "    %s = %s %s %s;\n",
              ins->result, a1_converted, op, a2_converted);
      append_code(&result, line);
      break;
    }

    // ============================================================
    // UNARY OPERATIONS
    // ============================================================
    case TAC_NOT:
      sprintf(line, "    %s = !(%s);\n", ins->result, ins->arg1);
      append_code(&result, line);
      break;

    case TAC_NEG:
      sprintf(line, "    %s = -(%s);\n", ins->result, ins->arg1);
      append_code(&result, line);
      break;

    // ============================================================
    // OUTPUT (PRINT)
    // ============================================================
    case TAC_OUTPUT:
    {
      const char *value = ins->arg1;

      if (is_string_literal(value))
      {
        // String output
        sprintf(line, "    printf(\"%%s\\n\", %s);\n", value);
      }
      else if (is_decimal_literal(value) || (value[0] == 't'))
      {
        // Double output
        sprintf(line, "    printf(\"%%g\\n\", (double)(%s));\n", value);
      }
      else if (is_integer_literal(value) || is_boolean_literal(value))
      {
        // Integer output
        sprintf(line, "    printf(\"%%d\\n\", %s);\n", value);
      }
      else
      {
        // Default: double output
        sprintf(line, "    printf(\"%%g\\n\", (double)(%s));\n", value);
      }
      append_code(&result, line);
      break;
    }

    // ============================================================
    // INPUT (READ)
    // ============================================================
    case TAC_INPUT:
    {
      sprintf(line,
              "    printf(\"%%s\", %s);\n"
              "    fgets(%s, 4096, stdin);\n"
              "    %s[strcspn(%s, \"\\n\")] = 0;\n",
              ins->arg1,   // Prompt string
              ins->result, // Buffer to store input
              ins->result, // Buffer name for strcspn
              ins->result  // Buffer name for strcspn
      );
      append_code(&result, line);
      break;
    }

    default:
      // Ignore unknown instructions
      break;
    }
  }

  // ============================================================
  // GENERATE MAIN FUNCTION RETURN
  // ============================================================
  append_code(&result, "    return 0;\n}\n");

  return result;
}