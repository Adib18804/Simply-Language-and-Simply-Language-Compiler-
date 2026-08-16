#ifndef TAC_H
#define TAC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// TAC CONFIGURATION
// ============================================================

#define MAX_TAC_INSTRS 512 // Maximum number of TAC instructions
#define MAX_TAC_STR 128    // Maximum length of TAC string operands

// ============================================================
// FORWARD DECLARATION
// ============================================================

typedef struct ASTNode ASTNode; // Forward declaration for AST integration

// ============================================================
// TAC OPERATION TYPES
// ============================================================

/**
 * Enumeration of all Three Address Code (TAC) operations
 * TAC is an intermediate representation used for code generation
 */
typedef enum
{
  // ============================================================
  // ASSIGNMENT
  // ============================================================
  TAC_ASSIGN, // result = arg1

  // ============================================================
  // ARITHMETIC OPERATIONS
  // ============================================================
  TAC_ADD, // result = arg1 + arg2
  TAC_SUB, // result = arg1 - arg2
  TAC_MUL, // result = arg1 * arg2
  TAC_DIV, // result = arg1 / arg2
  TAC_MOD, // result = arg1 % arg2

  // ============================================================
  // COMPARISON OPERATIONS
  // ============================================================
  TAC_EQ,  // result = arg1 == arg2
  TAC_NEQ, // result = arg1 != arg2
  TAC_LT,  // result = arg1 < arg2
  TAC_GT,  // result = arg1 > arg2
  TAC_LTE, // result = arg1 <= arg2
  TAC_GTE, // result = arg1 >= arg2

  // ============================================================
  // LOGICAL OPERATIONS
  // ============================================================
  TAC_AND, // result = arg1 && arg2
  TAC_OR,  // result = arg1 || arg2
  TAC_NOT, // result = !arg1 (unary)
  TAC_NEG, // result = -arg1 (unary negation)

  // ============================================================
  // I/O OPERATIONS
  // ============================================================
  TAC_OUTPUT, // show arg1
  TAC_INPUT,  // result = ask arg1 (prompt)

  // ============================================================
  // CONTROL FLOW OPERATIONS
  // ============================================================
  TAC_LABEL,         // label name: (target for jumps)
  TAC_JUMP,          // goto result (unconditional jump)
  TAC_JUMP_IF_FALSE, // if false arg1 goto result (conditional jump)

  // ============================================================
  // TASK/FUNCTION OPERATIONS
  // ============================================================
  TAC_TASK_DECL, // Task declaration (not fully implemented)
  TAC_TASK_CALL, // call arg1 (function call)
  TAC_RETURN     // return arg1 (return from function)
} TACOp;

// ============================================================
// TAC INSTRUCTION STRUCTURE
// ============================================================

/**
 * Structure representing a single TAC instruction
 *
 * TAC follows the format: result = arg1 op arg2
 * For unary operations: result = op arg1
 * For control flow: op arg1 (with result as target label)
 */
typedef struct
{
  TACOp op;                 // Operation type
  char result[MAX_TAC_STR]; // Result operand (destination)
  char arg1[MAX_TAC_STR];   // First argument (source)
  char arg2[MAX_TAC_STR];   // Second argument (source)
  int line;                 // Source line number (for debugging)
  int label_id;             // Label ID (for label instructions)
} TACInstr;

// ============================================================
// TAC PROGRAM STRUCTURE
// ============================================================

/**
 * Structure representing a complete TAC program
 * Contains an array of TAC instructions and state for generation
 */
typedef struct
{
  TACInstr instructions[MAX_TAC_INSTRS]; // Array of TAC instructions
  int count;                             // Number of instructions
  int temp_counter;                      // Counter for temporary variables
  int label_counter;                     // Counter for labels
} TACProgram;

// ============================================================
// TAC FUNCTIONS
// ============================================================

/**
 * Initialize a TAC program structure
 * @param program - Pointer to TACProgram to initialize
 */
void tac_init(TACProgram *program);

/**
 * Generate a new temporary variable name
 * @param program - TAC program
 * @param output_buffer - Buffer to store the temporary name
 * @return - Pointer to the temporary name
 *
 * Example:
 *   char temp[128];
 *   const char* t = tac_new_temp(prog, temp);
 *   // temp contains "t0", then "t1", etc.
 */
const char *tac_new_temp(TACProgram *program, char *output_buffer);

/**
 * Generate a new label ID
 * @param program - TAC program
 * @return - New label ID
 *
 * Example:
 *   int label = tac_new_label(prog);  // returns 0, then 1, etc.
 */
int tac_new_label(TACProgram *program);

/**
 * Emit a TAC instruction
 * @param program - TAC program
 * @param op      - TAC operation
 * @param result  - Result operand (can be NULL)
 * @param arg1    - First argument (can be NULL)
 * @param arg2    - Second argument (can be NULL)
 * @param line    - Source line number
 *
 * Examples:
 *   tac_emit(prog, TAC_ADD, "t0", "x", "5", line);
 *   // Generates: t0 = x + 5
 *
 *   tac_emit(prog, TAC_OUTPUT, NULL, "x", NULL, line);
 *   // Generates: show x
 */
void tac_emit(TACProgram *program, TACOp op, const char *result,
              const char *arg1, const char *arg2, int line);

/**
 * Generate TAC (Three Address Code) from an Abstract Syntax Tree (AST)
 * @param ast - Root of the Abstract Syntax Tree
 * @return    - TACProgram containing the generated TAC instructions
 *
 * Example Input AST:
 *   ASSIGNMENT
 *     IDENTIFIER 'x'
 *     BINARY_OP '+'
 *       INTEGER '5'
 *       INTEGER '3'
 *
 * Example Output TAC:
 *   t0 = 5 + 3
 *   x = t0
 */
TACProgram tac_generate(ASTNode *ast);

/**
 * Print TAC program for debugging purposes
 * @param program - TAC program to print
 */
void tac_print(TACProgram *program);

/**
 * Convert TAC operation to a human-readable string
 * @param op - TAC operation
 * @return   - String representation
 *
 * Examples:
 *   TAC_ADD  -> "+"
 *   TAC_SUB  -> "-"
 *   TAC_MUL  -> "*"
 *   TAC_DIV  -> "/"
 *   TAC_EQ   -> "=="
 *   TAC_OUTPUT -> "show"
 */
const char *tac_op_string(TACOp op);

#endif // TAC_H