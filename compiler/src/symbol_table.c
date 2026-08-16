#include "symbol_table.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// ============================================================
// SYMBOL TABLE INITIALIZATION
// ============================================================

/**
 * Initialize an empty symbol table
 * @param symbol_table - Pointer to SymbolTable to initialize
 */
void symbol_table_init(SymbolTable *symbol_table)
{
    symbol_table->count = 0;
    symbol_table->current_scope = 0;
}

// ============================================================
// SCOPE MANAGEMENT
// ============================================================

/**
 * Enter a new scope (e.g., inside a block, loop, or task)
 * @param symbol_table - Pointer to SymbolTable
 */
void symbol_table_enter_scope(SymbolTable *symbol_table)
{
    symbol_table->current_scope++;
}

/**
 * Exit the current scope and remove all symbols declared in it
 * @param symbol_table - Pointer to SymbolTable
 */
void symbol_table_exit_scope(SymbolTable *symbol_table)
{
    // Remove all symbols in the current scope (from end to start)
    for (int i = symbol_table->count - 1; i >= 0; i--)
    {
        if (symbol_table->symbols[i].scope_level == symbol_table->current_scope)
        {
            // Shift remaining symbols to fill the gap
            for (int j = i; j < symbol_table->count - 1; j++)
            {
                symbol_table->symbols[j] = symbol_table->symbols[j + 1];
            }
            symbol_table->count--;
        }
    }

    // Decrement scope level if possible
    if (symbol_table->current_scope > 0)
    {
        symbol_table->current_scope--;
    }
}

// ============================================================
// SYMBOL ADDITION
// ============================================================

/**
 * Add a new symbol to the symbol table
 * @param symbol_table - Pointer to SymbolTable
 * @param name         - Variable name
 * @param type         - Symbol type (INT, DECIMAL, STRING, BOOL, TASK)
 * @param line         - Declaration line number
 * @return             - 1 if added successfully, 0 if duplicate or table full
 */
int symbol_table_add(SymbolTable *symbol_table, const char *name,
                     SymbolType type, int line)
{
    // Check for duplicate declaration in the same scope
    for (int i = 0; i < symbol_table->count; i++)
    {
        if (symbol_table->symbols[i].scope_level == symbol_table->current_scope &&
            strcmp(symbol_table->symbols[i].name, name) == 0)
        {
            return 0; // Duplicate found
        }
    }

    // Check if symbol table is full
    if (symbol_table->count >= MAX_SYMBOLS)
    {
        return 0;
    }

    // Add new symbol
    Symbol *symbol = &symbol_table->symbols[symbol_table->count++];

    // Copy name (max 255 chars)
    strncpy(symbol->name, name, 255);
    symbol->name[255] = '\0';

    // Set symbol properties
    symbol->type = type;
    symbol->scope_level = symbol_table->current_scope;
    symbol->declared_line = line;

    return 1;
}

// ============================================================
// SYMBOL LOOKUP
// ============================================================

/**
 * Look up a symbol by name in the symbol table
 * Searches from innermost scope to outermost (reverse order)
 * @param symbol_table - Pointer to SymbolTable
 * @param name         - Variable name to look up
 * @return             - Pointer to Symbol if found, NULL otherwise
 */
Symbol *symbol_table_lookup(SymbolTable *symbol_table, const char *name)
{
    // Search from end to start (innermost to outermost scope)
    for (int i = symbol_table->count - 1; i >= 0; i--)
    {
        if (strcmp(symbol_table->symbols[i].name, name) == 0)
        {
            return &symbol_table->symbols[i];
        }
    }
    return NULL; // Symbol not found
}

// ============================================================
// TYPE INFERENCE FROM LITERAL VALUES
// ============================================================

/**
 * Infer the type of a literal value
 * @param value - String representation of the literal
 * @return      - Inferred SymbolType
 */
SymbolType symbol_table_infer_from_value(const char *value)
{
    // Check for null or empty
    if (!value || !value[0])
    {
        return SYM_UNKNOWN;
    }

    // String literal: starts with double quote
    if (value[0] == '"')
    {
        return SYM_STRING;
    }

    // Boolean literal
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
    {
        return SYM_BOOL;
    }

    // Check for numeric literal (integer or decimal)
    int has_decimal_point = 0;
    int start_index = (value[0] == '-') ? 1 : 0;

    for (int i = start_index; value[i] != '\0'; i++)
    {
        if (value[i] == '.')
        {
            // Only one decimal point allowed
            if (has_decimal_point)
            {
                return SYM_UNKNOWN;
            }
            has_decimal_point = 1;
        }
        else if (!isdigit(value[i]))
        {
            // Invalid character for numeric literal
            return SYM_UNKNOWN;
        }
    }

    // Return appropriate type
    return has_decimal_point ? SYM_DECIMAL : SYM_INT;
}

// ============================================================
// TYPE TO STRING CONVERSION
// ============================================================

/**
 * Convert SymbolType enum to a human-readable string
 * @param type - Symbol type
 * @return     - String representation
 */
const char *symbol_type_to_string(SymbolType type)
{
    switch (type)
    {
    case SYM_INT:
        return "INT";
    case SYM_DECIMAL:
        return "DECIMAL";
    case SYM_STRING:
        return "STRING";
    case SYM_BOOL:
        return "BOOL";
    case SYM_TASK:
        return "TASK";
    default:
        return "UNKNOWN";
    }
}