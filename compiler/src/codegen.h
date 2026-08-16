#ifndef CODEGEN_H
#define CODEGEN_H

#include "tac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// CODE GENERATOR CONFIGURATION
// ============================================================

#define CODEGEN_BUFFER_SIZE 65536 // Maximum size of generated code buffer
#define CODEGEN_ERROR_SIZE 1024   // Maximum size of error message buffer

// ============================================================
// CODE GENERATION RESULT STRUCTURE
// ============================================================

/**
 * Structure to hold the result of code generation
 * Contains the generated C code and status information
 */
typedef struct
{
    char code[CODEGEN_BUFFER_SIZE]; // Buffer for generated C code
    int length;                     // Current length of generated code
    int success;                    // Flag: 1 = success, 0 = failure
    char error[CODEGEN_ERROR_SIZE]; // Error message (if any)
} CodeGenResult;

// ============================================================
// CODE GENERATION FUNCTION
// ============================================================

/**
 * Generate C code from Three Address Code (TAC) intermediate representation
 *
 * @param tac - Pointer to TACProgram containing TAC instructions
 * @return    - CodeGenResult containing generated code and status
 *
 * The generated code is a complete C program with:
 *   - Required header includes (stdio.h, stdlib.h, string.h, stdbool.h)
 *   - Variable declarations with inferred types
 *   - Main function with all statements
 *   - Proper return statement
 */
CodeGenResult codegen_generate(TACProgram *tac);

#endif // CODEGEN_H