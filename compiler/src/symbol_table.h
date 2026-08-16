#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// SYMBOL TABLE CONFIGURATION
// ============================================================

#define MAX_SYMBOLS 128          // Maximum number of symbols in the table
#define MAX_SCOPE_DEPTH 16       // Maximum nesting depth of scopes
#define MAX_SYMBOL_NAME_LEN 256  // Maximum length of a symbol name

// ============================================================
// SYMBOL TYPES
// ============================================================

/**
 * Enumeration of all possible symbol types in the Simply language
 */
typedef enum {
    SYM_INT,        // Integer type (e.g., 42)
    SYM_DECIMAL,    // Decimal type (e.g., 3.14)
    SYM_STRING,     // String type (e.g., "hello")
    SYM_BOOL,       // Boolean type (true, false)
    SYM_TASK,       // Task/function type (user-defined function)
    SYM_UNKNOWN     // Unknown or unresolved type
} SymbolType;

// ============================================================
// SYMBOL STRUCTURE
// ============================================================

/**
 * Structure representing a single symbol in the symbol table
 * A symbol is a declared variable or task in the program
 */
typedef struct {
    char name[MAX_SYMBOL_NAME_LEN];  // Symbol name (identifier)
    SymbolType type;                 // Data type of the symbol
    int scope_level;                 // Scope nesting level where declared
    int declared_line;               // Line number where symbol was declared
} Symbol;

// ============================================================
// SYMBOL TABLE STRUCTURE
// ============================================================

/**
 * Structure representing the symbol table
 * Manages all declared symbols with scope information
 * 
 * Scope Rules:
 *   - Each block (conditional, loop, task) creates a new scope
 *   - Variables declared in inner scopes shadow outer variables
 *   - Variables in outer scopes are visible in inner scopes
 *   - Variables in inner scopes are NOT visible in outer scopes
 */
typedef struct {
    Symbol symbols[MAX_SYMBOLS];     // Array of symbols
    int count;                       // Number of symbols currently stored
    int current_scope;               // Current scope level (0 = global)
} SymbolTable;

// ============================================================
// SYMBOL TABLE FUNCTIONS
// ============================================================

/**
 * Initialize an empty symbol table
 * @param symbol_table - Pointer to SymbolTable to initialize
 */
void symbol_table_init(SymbolTable* symbol_table);

/**
 * Enter a new scope (e.g., inside a block, loop, or task)
 * @param symbol_table - Pointer to SymbolTable
 */
void symbol_table_enter_scope(SymbolTable* symbol_table);

/**
 * Exit the current scope and remove all symbols declared in it
 * @param symbol_table - Pointer to SymbolTable
 */
void symbol_table_exit_scope(SymbolTable* symbol_table);

/**
 * Add a new symbol to the symbol table
 * @param symbol_table - Pointer to SymbolTable
 * @param name         - Variable name (identifier)
 * @param type         - Symbol type (INT, DECIMAL, STRING, BOOL, TASK)
 * @param line         - Declaration line number
 * @return             - 1 if added successfully, 0 if duplicate or table full
 * 
 * Example:
 *   symbol_table_add(st, "x", SYM_INT, 5);
 *   Adds variable 'x' of type INT at line 5 in current scope
 */
int symbol_table_add(SymbolTable* symbol_table, const char* name, 
                     SymbolType type, int line);

/**
 * Look up a symbol by name in the symbol table
 * Searches from innermost scope to outermost (reverse order)
 * @param symbol_table - Pointer to SymbolTable
 * @param name         - Variable name to look up
 * @return             - Pointer to Symbol if found, NULL otherwise
 * 
 * Example:
 *   Symbol* s = symbol_table_lookup(st, "x");
 *   Returns the symbol for 'x' if declared in any scope
 */
Symbol* symbol_table_lookup(SymbolTable* symbol_table, const char* name);

/**
 * Infer the type of a literal value
 * @param value - String representation of the literal
 * @return      - Inferred SymbolType
 * 
 * Examples:
 *   "42"     -> SYM_INT
 *   "3.14"   -> SYM_DECIMAL
 *   "hello"  -> SYM_STRING
 *   "true"   -> SYM_BOOL
 *   "false"  -> SYM_BOOL
 */
SymbolType symbol_table_infer_from_value(const char* value);

/**
 * Convert SymbolType enum to a human-readable string
 * @param type - Symbol type
 * @return     - String representation
 * 
 * Examples:
 *   SYM_INT     -> "INT"
 *   SYM_DECIMAL -> "DECIMAL"
 *   SYM_STRING  -> "STRING"
 *   SYM_BOOL    -> "BOOL"
 *   SYM_TASK    -> "TASK"
 *   SYM_UNKNOWN -> "UNKNOWN"
 */
const char* symbol_type_to_string(SymbolType type);

#endif // SYMBOL_TABLE_H