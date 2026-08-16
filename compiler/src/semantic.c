#include "semantic.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// SEMANTIC ANALYSIS CONFIGURATION
// ============================================================

#define MAX_SEMANTIC_ERRORS 256     // Maximum number of semantic errors to store

// ============================================================
// SEMANTIC ANALYSIS STATE
// ============================================================

static SemanticResult semantic_result;

// ============================================================
// ERROR HANDLING
// ============================================================

/**
 * Add a semantic error to the result
 */
static void add_semantic_error(const char* error_type, const char* message, int line) {
    // Check if maximum errors reached
    if (semantic_result.error_count >= MAX_SEMANTIC_ERRORS) {
        return;
    }
    
    SemanticError* error = &semantic_result.errors[semantic_result.error_count++];
    
    // Copy error type (max 63 chars)
    strncpy(error->error_type, error_type, 63);
    error->error_type[63] = '\0';
    
    // Copy error message (max 1023 chars)
    strncpy(error->message, message, 1023);
    error->message[1023] = '\0';
    
    error->line = line;
    semantic_result.success = 0;  // Mark as failed
}

// ============================================================
// TYPE INFERENCE
// ============================================================

/**
 * Infer the type of an AST node
 * @param node - AST node to analyze
 * @return     - Inferred SymbolType
 */
static SymbolType infer_ast_type(ASTNode* node) {
    if (!node) {
        return SYM_UNKNOWN;
    }
    
    switch (node->type) {
        // ============================================================
        // LITERAL NODES
        // ============================================================
        case NODE_INTEGER:
            return SYM_INT;
            
        case NODE_DECIMAL:
            return SYM_DECIMAL;
            
        case NODE_STRING:
            return SYM_STRING;
            
        case NODE_BOOLEAN:
            return SYM_BOOL;
        
        // ============================================================
        // IDENTIFIER NODE - Look up in symbol table
        // ============================================================
        case NODE_IDENTIFIER: {
            Symbol* symbol = symbol_table_lookup(&semantic_result.symbol_table, 
                                                  node->value);
            return symbol ? symbol->type : SYM_UNKNOWN;
        }
        
        // ============================================================
        // BINARY OPERATION - Infer from operands
        // ============================================================
        case NODE_BINARY_OP: {
            // Ensure we have at least 2 children
            if (node->children_count < 2) {
                return SYM_UNKNOWN;
            }
            
            SymbolType left_type = infer_ast_type(node->children[0]);
            SymbolType right_type = infer_ast_type(node->children[1]);
            
            // String concatenation
            if (left_type == SYM_STRING || right_type == SYM_STRING) {
                return SYM_STRING;
            }
            
            // Decimal (floating point) takes precedence
            if (left_type == SYM_DECIMAL || right_type == SYM_DECIMAL) {
                return SYM_DECIMAL;
            }
            
            // Boolean
            if (left_type == SYM_BOOL || right_type == SYM_BOOL) {
                return SYM_BOOL;
            }
            
            // Integer
            if (left_type == SYM_INT && right_type == SYM_INT) {
                return SYM_INT;
            }
            
            return SYM_UNKNOWN;
        }
        
        // ============================================================
        // UNARY OPERATION - Type same as operand
        // ============================================================
        case NODE_UNARY_OP: {
            if (node->children_count < 1) {
                return SYM_UNKNOWN;
            }
            return infer_ast_type(node->children[0]);
        }
        
        default:
            return SYM_UNKNOWN;
    }
}

// ============================================================
// SEMANTIC ANALYSIS - NODE VISITOR
// ============================================================

/**
 * Recursively visit AST nodes for semantic analysis
 * Performs:
 *   - Variable declaration checking
 *   - Type checking
 *   - Scope management
 *   - Operation validity checking
 */
static void visit_node(ASTNode* node) {
    if (!node) {
        return;
    }
    
    char error_buffer[1024];
    
    switch (node->type) {
        // ============================================================
        // DECLARATION: make x = 10
        // ============================================================
        case NODE_DECLARATION: {
            if (node->children_count >= 1) {
                const char* variable_name = node->children[0]->value;
                int line = node->line;
                
                // Infer type from initializer if present
                SymbolType variable_type = SYM_UNKNOWN;
                if (node->children_count >= 2) {
                    variable_type = infer_ast_type(node->children[1]);
                }
                
                // Add to symbol table
                if (!symbol_table_add(&semantic_result.symbol_table, 
                                      variable_name, variable_type, line)) {
                    snprintf(error_buffer, 1024, 
                             "Variable '%s' is already declared in this scope.", 
                             variable_name);
                    add_semantic_error("DUPLICATE_DECLARATION", error_buffer, line);
                }
            }
            
            // Visit children (skip first child - identifier already processed)
            for (int i = 1; i < node->children_count; i++) {
                visit_node(node->children[i]);
            }
            return;
        }
        
        // ============================================================
        // ASSIGNMENT: x = 10
        // ============================================================
        case NODE_ASSIGNMENT: {
            if (node->children_count >= 1) {
                const char* variable_name = node->children[0]->value;
                
                // Check if variable is declared
                Symbol* symbol = symbol_table_lookup(&semantic_result.symbol_table, 
                                                     variable_name);
                if (!symbol) {
                    snprintf(error_buffer, 1024, 
                             "Variable '%s' has not been declared.", 
                             variable_name);
                    add_semantic_error("UNDECLARED_VARIABLE", error_buffer, node->line);
                } 
                // Check type compatibility
                else if (node->children_count >= 2) {
                    SymbolType right_type = infer_ast_type(node->children[1]);
                    
                    if (symbol->type != SYM_UNKNOWN && right_type != SYM_UNKNOWN) {
                        int type_mismatch = 0;
                        
                        // String can only be assigned from string
                        if (symbol->type == SYM_STRING && right_type != SYM_STRING) {
                            type_mismatch = 1;
                        }
                        // Bool can be assigned from bool or int
                        else if (symbol->type == SYM_BOOL && 
                                 right_type != SYM_BOOL && right_type != SYM_INT) {
                            type_mismatch = 1;
                        }
                        // Int cannot be assigned from string
                        else if (symbol->type == SYM_INT && right_type == SYM_STRING) {
                            type_mismatch = 1;
                        }
                        
                        if (type_mismatch) {
                            snprintf(error_buffer, 1024,
                                     "Type mismatch: cannot assign %s to variable '%s' of type %s.",
                                     symbol_type_to_string(right_type),
                                     variable_name,
                                     symbol_type_to_string(symbol->type));
                            add_semantic_error("TYPE_MISMATCH", error_buffer, node->line);
                        }
                    }
                }
            }
            
            // Visit children (skip first child - identifier already processed)
            for (int i = 1; i < node->children_count; i++) {
                visit_node(node->children[i]);
            }
            return;
        }
        
        // ============================================================
        // IDENTIFIER REFERENCE
        // ============================================================
        case NODE_IDENTIFIER: {
            Symbol* symbol = symbol_table_lookup(&semantic_result.symbol_table, 
                                                  node->value);
            if (!symbol) {
                snprintf(error_buffer, 1024, 
                         "Variable '%s' has not been declared.", 
                         node->value);
                add_semantic_error("UNDECLARED_VARIABLE", error_buffer, node->line);
            }
            return;
        }
        
        // ============================================================
        // CONDITIONAL, LOOP, TASK - Create new scope
        // ============================================================
        case NODE_CONDITIONAL:
        case NODE_LOOP:
        case NODE_TASK_DECL: {
            // Enter new scope for block
            symbol_table_enter_scope(&semantic_result.symbol_table);
            
            // Visit all children
            for (int i = 0; i < node->children_count; i++) {
                visit_node(node->children[i]);
            }
            
            // Exit scope
            symbol_table_exit_scope(&semantic_result.symbol_table);
            return;
        }
        
        // ============================================================
        // INPUT: ask "prompt" x
        // ============================================================
        case NODE_INPUT: {
            if (node->children_count >= 2) {
                const char* variable_name = node->children[1]->value;
                
                Symbol* symbol = symbol_table_lookup(&semantic_result.symbol_table, 
                                                     variable_name);
                if (!symbol) {
                    snprintf(error_buffer, 1024, 
                             "Variable '%s' has not been declared.", 
                             variable_name);
                    add_semantic_error("UNDECLARED_VARIABLE", error_buffer, node->line);
                }
            }
            return;
        }
        
        // ============================================================
        // BINARY OPERATION - Check operator validity
        // ============================================================
        case NODE_BINARY_OP: {
            if (node->children_count >= 2) {
                SymbolType left_type = infer_ast_type(node->children[0]);
                SymbolType right_type = infer_ast_type(node->children[1]);
                
                // Check if operator is valid for strings
                // String concatenation only supports '+'
                if (strcmp(node->op_type, "OR") != 0 && 
                    strcmp(node->op_type, "AND") != 0 &&
                    strcmp(node->op_type, "EQ") != 0 &&
                    strcmp(node->op_type, "NEQ") != 0) {
                    
                    if (left_type == SYM_STRING || right_type == SYM_STRING) {
                        snprintf(error_buffer, 1024,
                                 "Operator '%s' cannot be applied to strings.",
                                 node->value);
                        add_semantic_error("INVALID_OPERATION", error_buffer, node->line);
                    }
                }
            }
            
            // Visit all children
            for (int i = 0; i < node->children_count; i++) {
                visit_node(node->children[i]);
            }
            return;
        }
        
        // ============================================================
        // DEFAULT - Visit all children
        // ============================================================
        default:
            for (int i = 0; i < node->children_count; i++) {
                visit_node(node->children[i]);
            }
            return;
    }
}

// ============================================================
// MAIN SEMANTIC ANALYSIS ENTRY POINT
// ============================================================

/**
 * Perform semantic analysis on an AST
 * 
 * Checks:
 *   - All variables are declared before use
 *   - No duplicate declarations in same scope
 *   - Type compatibility in assignments
 *   - Valid operations on types
 *   - Proper scope handling
 * 
 * @param ast - Root of the Abstract Syntax Tree
 * @return    - SemanticResult containing:
 *                - symbol_table: All symbols with their types
 *                - errors: Array of semantic errors
 *                - success: Boolean success flag
 */
SemanticResult semantic_analyze(ASTNode* ast) {
    // Initialize result
    semantic_result.success = 1;
    semantic_result.error_count = 0;
    symbol_table_init(&semantic_result.symbol_table);
    
    // Visit all nodes in the AST
    if (ast) {
        visit_node(ast);
    }
    
    return semantic_result;
}