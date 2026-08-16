#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

// ============================================================
// SEMANTIC ANALYSIS CONFIGURATION
// ============================================================

#define MAX_SEMANTIC_ERRORS 256     // Maximum number of semantic errors to store
#define MAX_ERROR_MESSAGE_LEN 1024  // Maximum length of error message
#define MAX_ERROR_TYPE_LEN 64       // Maximum length of error type string

// ============================================================
// SEMANTIC ERROR STRUCTURE
// ============================================================

/**
 * Structure representing a single semantic error
 */
typedef struct {
    char message[MAX_ERROR_MESSAGE_LEN];  // Human-readable error message
    int line;                             // Line number where error occurred
    char error_type[MAX_ERROR_TYPE_LEN];  // Error category (e.g., "TYPE_MISMATCH")
} SemanticError;

// ============================================================
// SEMANTIC RESULT STRUCTURE
// ============================================================

/**
 * Structure containing the result of semantic analysis
 */
typedef struct {
    SymbolTable symbol_table;              // Symbol table with all declared symbols
    SemanticError errors[MAX_SEMANTIC_ERRORS];  // Array of semantic errors
    int error_count;                       // Number of errors encountered
    int success;                           // 1 if analysis succeeded, 0 if errors occurred
} SemanticResult;

// ============================================================
// SEMANTIC ANALYSIS FUNCTION
// ============================================================

/**
 * Perform semantic analysis on an Abstract Syntax Tree (AST)
 * 
 * Performs the following checks:
 *   - Variable Declaration: All variables must be declared before use
 *   - Duplicate Declaration: Variables cannot be declared twice in the same scope
 *   - Type Checking: Assignment types must be compatible
 *   - Scope Management: Variables respect lexical scoping rules
 *   - Operation Validity: Operators are valid for operand types
 * 
 * The analysis traverses the AST and builds a symbol table with:
 *   - Variable names
 *   - Inferred types (INT, DECIMAL, STRING, BOOL)
 *   - Scope levels
 *   - Declaration line numbers
 * 
 * @param ast - Root of the Abstract Syntax Tree
 * @return    - SemanticResult containing:
 *                - symbol_table: All symbols with their types and scopes
 *                - errors: Array of semantic errors (if any)
 *                - error_count: Number of errors
 *                - success: Boolean success flag
 * 
 * Example:
 *   Input AST:
 *     DECLARATION
 *       IDENTIFIER 'x'
 *       INTEGER '10'
 *   
 *   Output Symbol Table:
 *     x | INT | scope=0 | line=1
 * 
 *   Input AST (with error):
 *     ASSIGNMENT
 *       IDENTIFIER 'x'
 *       INTEGER '10'
 *   
 *   Output Error:
 *     UNDECLARED_VARIABLE: Variable 'x' has not been declared. (line 1)
 */
SemanticResult semantic_analyze(ASTNode* ast);

#endif // SEMANTIC_H