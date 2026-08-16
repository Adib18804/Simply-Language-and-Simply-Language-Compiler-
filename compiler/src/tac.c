#include "tac.h"
#include "ast.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// TAC GENERATOR STATE
// ============================================================

static TACProgram *current_program; // Current TAC program being generated

// ============================================================
// TAC PROGRAM INITIALIZATION
// ============================================================

/**
 * Initialize a TAC program structure
 */
void tac_init(TACProgram *program)
{
  program->count = 0;
  program->temp_counter = 0;
  program->label_counter = 0;
}

// ============================================================
// TEMPORARY VARIABLE AND LABEL MANAGEMENT
// ============================================================

/**
 * Generate a new temporary variable name
 * @param program - TAC program
 * @param output_buffer - Buffer to store the temporary name
 * @return - Pointer to the temporary name
 */
const char *tac_new_temp(TACProgram *program, char *output_buffer)
{
  sprintf(output_buffer, "t%d", program->temp_counter++);
  return output_buffer;
}

/**
 * Generate a new label ID
 * @param program - TAC program
 * @return - New label ID
 */
int tac_new_label(TACProgram *program)
{
  return program->label_counter++;
}

// ============================================================
// TAC INSTRUCTION EMISSION
// ============================================================

/**
 * Emit a TAC instruction
 * @param program - TAC program
 * @param op      - TAC operation
 * @param result  - Result operand (can be NULL)
 * @param arg1    - First argument (can be NULL)
 * @param arg2    - Second argument (can be NULL)
 * @param line    - Source line number
 */
void tac_emit(TACProgram *program, TACOp op, const char *result,
              const char *arg1, const char *arg2, int line)
{
  // Check if instruction buffer is full
  if (program->count >= MAX_TAC_INSTRS)
  {
    return;
  }

  TACInstr *instruction = &program->instructions[program->count++];

  // Set instruction properties
  instruction->op = op;
  instruction->line = line;
  instruction->label_id = -1;

  // Copy result operand
  if (result != NULL)
  {
    strncpy(instruction->result, result, MAX_TAC_STR - 1);
    instruction->result[MAX_TAC_STR - 1] = '\0';
  }
  else
  {
    instruction->result[0] = '\0';
  }

  // Copy first argument
  if (arg1 != NULL)
  {
    strncpy(instruction->arg1, arg1, MAX_TAC_STR - 1);
    instruction->arg1[MAX_TAC_STR - 1] = '\0';
  }
  else
  {
    instruction->arg1[0] = '\0';
  }

  // Copy second argument
  if (arg2 != NULL)
  {
    strncpy(instruction->arg2, arg2, MAX_TAC_STR - 1);
    instruction->arg2[MAX_TAC_STR - 1] = '\0';
  }
  else
  {
    instruction->arg2[0] = '\0';
  }
}

// ============================================================
// LABEL EMISSION
// ============================================================

/**
 * Emit a label instruction
 * @param program   - TAC program
 * @param label_id  - Label ID to emit
 */
static void emit_label(TACProgram *program, int label_id)
{
  if (program->count >= MAX_TAC_INSTRS)
  {
    return;
  }

  TACInstr *instruction = &program->instructions[program->count++];
  instruction->op = TAC_LABEL;
  instruction->line = 0;
  instruction->label_id = label_id;

  // Generate label name
  char label_buffer[64];
  sprintf(label_buffer, "L%d", label_id);
  strcpy(instruction->result, label_buffer);

  instruction->arg1[0] = '\0';
  instruction->arg2[0] = '\0';
}

// ============================================================
// AST TO TAC CONVERSION HELPERS
// ============================================================

/**
 * Convert binary operator string to TAC operation
 */
static TACOp binary_op_to_tac(const char *op_type)
{
  if (!strcmp(op_type, "PLUS"))
    return TAC_ADD;
  if (!strcmp(op_type, "MINUS"))
    return TAC_SUB;
  if (!strcmp(op_type, "MUL"))
    return TAC_MUL;
  if (!strcmp(op_type, "DIV"))
    return TAC_DIV;
  if (!strcmp(op_type, "MOD"))
    return TAC_MOD;
  if (!strcmp(op_type, "EQ"))
    return TAC_EQ;
  if (!strcmp(op_type, "NEQ"))
    return TAC_NEQ;
  if (!strcmp(op_type, "LT"))
    return TAC_LT;
  if (!strcmp(op_type, "GT"))
    return TAC_GT;
  if (!strcmp(op_type, "LTE"))
    return TAC_LTE;
  if (!strcmp(op_type, "GTE"))
    return TAC_GTE;
  if (!strcmp(op_type, "AND"))
    return TAC_AND;
  if (!strcmp(op_type, "OR"))
    return TAC_OR;

  return TAC_ADD; // Default fallback
}

// ============================================================
// EXPRESSION GENERATION
// ============================================================

/**
 * Generate TAC for an expression
 * @param node - AST node for the expression
 * @param output_buffer - Buffer to store the result location
 * @return - Pointer to the result location
 */
static char *generate_expression(ASTNode *node, char *output_buffer)
{
  if (!node)
  {
    output_buffer[0] = '\0';
    return output_buffer;
  }

  switch (node->type)
  {
  // ============================================================
  // LITERAL AND IDENTIFIER NODES
  // ============================================================
  case NODE_INTEGER:
  case NODE_DECIMAL:
  case NODE_BOOLEAN:
  case NODE_IDENTIFIER:
    strncpy(output_buffer, node->value, MAX_TAC_STR - 1);
    output_buffer[MAX_TAC_STR - 1] = '\0';
    return output_buffer;

  case NODE_STRING:
  {
    static char buffer[MAX_TAC_STR];
    snprintf(buffer, MAX_TAC_STR, "\"%s\"", node->value);
    strncpy(output_buffer, buffer, MAX_TAC_STR - 1);
    output_buffer[MAX_TAC_STR - 1] = '\0';
    return output_buffer;
  }

  // ============================================================
  // UNARY OPERATION
  // ============================================================
  case NODE_UNARY_OP:
  {
    char operand[MAX_TAC_STR];
    generate_expression(node->children[0], operand);

    char temp[MAX_TAC_STR];
    tac_new_temp(current_program, temp);

    // Determine unary operation type
    TACOp unary_op = (strcmp(node->op_type, "NOT") == 0) ? TAC_NOT : TAC_NEG;
    tac_emit(current_program, unary_op, temp, operand, NULL, node->line);

    strncpy(output_buffer, temp, MAX_TAC_STR - 1);
    output_buffer[MAX_TAC_STR - 1] = '\0';
    return output_buffer;
  }

  // ============================================================
  // BINARY OPERATION
  // ============================================================
  case NODE_BINARY_OP:
  {
    char left_operand[MAX_TAC_STR];
    char right_operand[MAX_TAC_STR];

    generate_expression(node->children[0], left_operand);
    generate_expression(node->children[1], right_operand);

    char temp[MAX_TAC_STR];
    tac_new_temp(current_program, temp);

    TACOp binary_op = binary_op_to_tac(node->op_type);
    tac_emit(current_program, binary_op, temp, left_operand, right_operand, node->line);

    strncpy(output_buffer, temp, MAX_TAC_STR - 1);
    output_buffer[MAX_TAC_STR - 1] = '\0';
    return output_buffer;
  }

  default:
    output_buffer[0] = '\0';
    return output_buffer;
  }
}

// ============================================================
// STATEMENT LIST GENERATION
// ============================================================

/**
 * Generate TAC for a list of statements
 */
static void generate_statement_list(ASTNode *statement_list)
{
  if (!statement_list)
  {
    return;
  }

  for (int i = 0; i < statement_list->children_count; i++)
  {
    ASTNode *statement = statement_list->children[i];
    if (!statement)
    {
      continue;
    }

    switch (statement->type)
    {
    // ============================================================
    // DECLARATION AND ASSIGNMENT
    // ============================================================
    case NODE_DECLARATION:
    case NODE_ASSIGNMENT:
    {
      if (statement->children_count >= 2)
      {
        const char *variable = statement->children[0]->value;
        char value[MAX_TAC_STR];
        generate_expression(statement->children[1], value);
        tac_emit(current_program, TAC_ASSIGN, variable, value, NULL, statement->line);
      }
      break;
    }

    // ============================================================
    // OUTPUT (SHOW)
    // ============================================================
    case NODE_OUTPUT:
    {
      if (statement->children_count >= 1)
      {
        char value[MAX_TAC_STR];
        generate_expression(statement->children[0], value);
        tac_emit(current_program, TAC_OUTPUT, NULL, value, NULL, statement->line);
      }
      break;
    }

    // ============================================================
    // INPUT (ASK)
    // ============================================================
    case NODE_INPUT:
    {
      if (statement->children_count >= 2)
      {
        const char *variable = statement->children[1]->value;
        char prompt[MAX_TAC_STR];
        snprintf(prompt, MAX_TAC_STR, "\"%s\"", statement->children[0]->value);
        tac_emit(current_program, TAC_INPUT, variable, prompt, NULL, statement->line);
      }
      break;
    }

    // ============================================================
    // CONDITIONAL (CHECK...OTHERWISE...FINISH)
    // ============================================================
    case NODE_CONDITIONAL:
    {
      if (statement->children_count >= 3)
      {
        int else_label = tac_new_label(current_program);
        int end_label = tac_new_label(current_program);

        // Generate condition
        char condition[MAX_TAC_STR];
        generate_expression(statement->children[0], condition);

        // Jump to else if condition is false
        tac_emit(current_program, TAC_JUMP_IF_FALSE, "", condition, "", statement->line);

        // Patch the jump target to else label
        TACInstr *jump_instruction = &current_program->instructions[current_program->count - 1];
        char label_buffer[64];
        sprintf(label_buffer, "L%d", else_label);
        strcpy(jump_instruction->result, label_buffer);

        // Generate if-block
        generate_statement_list(statement->children[1]);

        // Jump to end after if-block
        TACInstr jump_to_end;
        jump_to_end.op = TAC_JUMP;
        sprintf(label_buffer, "L%d", end_label);
        strcpy(jump_to_end.result, label_buffer);
        jump_to_end.arg1[0] = '\0';
        jump_to_end.arg2[0] = '\0';
        jump_to_end.line = statement->line;
        jump_to_end.label_id = -1;
        if (current_program->count < MAX_TAC_INSTRS)
        {
          current_program->instructions[current_program->count++] = jump_to_end;
        }

        // Else label
        emit_label(current_program, else_label);

        // Generate else-block
        generate_statement_list(statement->children[2]);

        // End label
        emit_label(current_program, end_label);
      }
      break;
    }

    // ============================================================
    // LOOP (REPEAT...FINISH)
    // ============================================================
    case NODE_LOOP:
    {
      if (statement->children_count >= 2)
      {
        int start_label = tac_new_label(current_program);
        int end_label = tac_new_label(current_program);
        const char *counter_variable = "__rc";

        // Initialize counter
        char count[MAX_TAC_STR];
        generate_expression(statement->children[0], count);
        tac_emit(current_program, TAC_ASSIGN, counter_variable, count, NULL, statement->line);

        // Start label
        emit_label(current_program, start_label);

        // Check if counter > 0
        const char *zero = "0";
        char temp[MAX_TAC_STR];
        tac_new_temp(current_program, temp);
        tac_emit(current_program, TAC_LT, temp, zero, counter_variable, statement->line);

        // Jump to end if counter <= 0
        tac_emit(current_program, TAC_JUMP_IF_FALSE, "", temp, "", statement->line);

        // Patch the jump target to end label
        TACInstr *jump_instruction = &current_program->instructions[current_program->count - 1];
        char label_buffer[64];
        sprintf(label_buffer, "L%d", end_label);
        strcpy(jump_instruction->result, label_buffer);

        // Generate loop body
        generate_statement_list(statement->children[1]);

        // Decrement counter
        const char *one = "1";
        char temp2[MAX_TAC_STR];
        tac_new_temp(current_program, temp2);
        tac_emit(current_program, TAC_SUB, temp2, counter_variable, one, statement->line);
        tac_emit(current_program, TAC_ASSIGN, counter_variable, temp2, NULL, statement->line);

        // Jump back to start
        TACInstr jump_to_start;
        jump_to_start.op = TAC_JUMP;
        sprintf(label_buffer, "L%d", start_label);
        strcpy(jump_to_start.result, label_buffer);
        jump_to_start.arg1[0] = '\0';
        jump_to_start.arg2[0] = '\0';
        jump_to_start.line = statement->line;
        jump_to_start.label_id = -1;
        if (current_program->count < MAX_TAC_INSTRS)
        {
          current_program->instructions[current_program->count++] = jump_to_start;
        }

        // End label
        emit_label(current_program, end_label);
      }
      break;
    }

    // ============================================================
    // TASK DECLARATION (Not fully implemented)
    // ============================================================
    case NODE_TASK_DECL:
    {
      // Task declarations are not yet implemented in TAC generation
      break;
    }

    // ============================================================
    // TASK CALL
    // ============================================================
    case NODE_TASK_CALL:
    {
      tac_emit(current_program, TAC_TASK_CALL, NULL, statement->value, NULL, statement->line);
      break;
    }

    // ============================================================
    // GIVE (RETURN)
    // ============================================================
    case NODE_GIVE:
    {
      if (statement->children_count >= 1)
      {
        char value[MAX_TAC_STR];
        generate_expression(statement->children[0], value);
        tac_emit(current_program, TAC_RETURN, NULL, value, NULL, statement->line);
      }
      break;
    }

    default:
      break;
    }
  }
}

// ============================================================
// MAIN TAC GENERATION ENTRY POINT
// ============================================================

/**
 * Generate TAC (Three Address Code) from an AST
 *
 * @param ast - Root of the Abstract Syntax Tree
 * @return    - TACProgram containing the generated TAC instructions
 */
TACProgram tac_generate(ASTNode *ast)
{
  TACProgram program;
  tac_init(&program);
  current_program = &program;

  // Generate TAC for the program
  if (ast && ast->children_count >= 1)
  {
    generate_statement_list(ast->children[0]);
  }

  return program;
}

// ============================================================
// TAC STRING CONVERSION
// ============================================================

/**
 * Convert TAC operation to a human-readable string
 */
const char *tac_op_string(TACOp op)
{
  switch (op)
  {
  case TAC_ASSIGN:
    return "=";
  case TAC_ADD:
    return "+";
  case TAC_SUB:
    return "-";
  case TAC_MUL:
    return "*";
  case TAC_DIV:
    return "/";
  case TAC_MOD:
    return "%";
  case TAC_EQ:
    return "==";
  case TAC_NEQ:
    return "!=";
  case TAC_LT:
    return "<";
  case TAC_GT:
    return ">";
  case TAC_LTE:
    return "<=";
  case TAC_GTE:
    return ">=";
  case TAC_AND:
    return "&&";
  case TAC_OR:
    return "||";
  case TAC_NOT:
    return "!";
  case TAC_NEG:
    return "-u";
  case TAC_OUTPUT:
    return "show";
  case TAC_INPUT:
    return "ask";
  case TAC_LABEL:
    return "label";
  case TAC_JUMP:
    return "goto";
  case TAC_JUMP_IF_FALSE:
    return "goto_if_false";
  case TAC_TASK_DECL:
    return "task";
  case TAC_TASK_CALL:
    return "call";
  case TAC_RETURN:
    return "return";
  default:
    return "?";
  }
}

// ============================================================
// TAC PRINTING
// ============================================================

/**
 * Print TAC program for debugging purposes
 */
void tac_print(TACProgram *program)
{
  for (int i = 0; i < program->count; i++)
  {
    TACInstr *instruction = &program->instructions[i];

    switch (instruction->op)
    {
    case TAC_LABEL:
      printf("%s:\n", instruction->result);
      break;

    case TAC_JUMP:
      printf("   goto %s\n", instruction->result);
      break;

    case TAC_JUMP_IF_FALSE:
      printf("   if false %s goto %s\n", instruction->arg1, instruction->result);
      break;

    case TAC_OUTPUT:
      printf("   show %s\n", instruction->arg1);
      break;

    case TAC_INPUT:
      printf("   %s = ask %s\n", instruction->result, instruction->arg1);
      break;

    case TAC_ASSIGN:
      printf("   %s = %s\n", instruction->result, instruction->arg1);
      break;

    case TAC_NOT:
    case TAC_NEG:
      printf("   %s = %s %s\n", instruction->result,
             tac_op_string(instruction->op), instruction->arg1);
      break;

    default:
      printf("   %s = %s %s %s\n", instruction->result,
             instruction->arg1, tac_op_string(instruction->op), instruction->arg2);
      break;
    }
  }
}