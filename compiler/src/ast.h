#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// CONSTANTS AND CONFIGURATIONS
// ============================================================

#define MAX_CHILDREN 32   // Maximum number of children per AST node
#define MAX_AST_NODES 512 // Maximum number of nodes in the AST pool

// ============================================================
// AST NODE TYPES
// ============================================================

typedef enum
{
    // Program structure
    NODE_PROGRAM,        // Root node representing entire program
    NODE_STATEMENT_LIST, // List of statements

    // Declarations and assignments
    NODE_DECLARATION, // Variable declaration (make x = 10)
    NODE_ASSIGNMENT,  // Variable assignment (x = 20)

    // I/O operations
    NODE_OUTPUT, // Output statement (show expression)
    NODE_INPUT,  // Input statement (ask "prompt" variable)

    // Control flow
    NODE_CONDITIONAL, // Conditional statement (check ... otherwise ... finish)
    NODE_LOOP,        // Loop statement (repeat ... finish)

    // Task/Function operations
    NODE_TASK_DECL, // Task declaration (task name() ... finish)
    NODE_TASK_CALL, // Task call (name())
    NODE_GIVE,      // Return statement (give expression)

    // Expressions
    NODE_BINARY_OP, // Binary operation (+, -, *, /, %, etc.)
    NODE_UNARY_OP,  // Unary operation (!, -)

    // Literal values
    NODE_INTEGER,   // Integer literal (42)
    NODE_DECIMAL,   // Decimal literal (3.14)
    NODE_STRING,    // String literal ("hello")
    NODE_BOOLEAN,   // Boolean literal (true, false)
    NODE_IDENTIFIER // Variable name (x, age, total)
} ASTNodeType;

// ============================================================
// AST NODE STRUCTURE
// ============================================================

typedef struct ASTNode
{
    ASTNodeType type;                       // Type of this AST node
    char value[256];                        // Value (for identifiers, literals)
    char op_type[64];                       // Operator type (for binary/unary ops)
    int line;                               // Source line number for error reporting
    int children_count;                     // Number of child nodes
    struct ASTNode *children[MAX_CHILDREN]; // Array of child nodes
} ASTNode;

// ============================================================
// AST FUNCTIONS
// ============================================================

/**
 * Create a new AST node
 * @param type  - Type of the node (ASTNodeType)
 * @param value - String value (NULL if not applicable)
 * @param line  - Source line number
 * @return      - Pointer to the newly created node, or NULL on failure
 */
ASTNode *ast_create_node(ASTNodeType type, const char *value, int line);

/**
 * Add a child node to a parent node
 * @param parent - Parent node
 * @param child  - Child node to add
 */
void ast_add_child(ASTNode *parent, ASTNode *child);

/**
 * Free the entire AST (reset the node pool)
 * @param root - Root node of the AST (unused, kept for API consistency)
 */
void ast_free(ASTNode *root);

/**
 * Print the AST tree (for debugging purposes)
 * @param node  - Current node to print
 * @param depth - Current depth in the tree (for indentation)
 */
void ast_print(ASTNode *node, int depth);

/**
 * Convert ASTNodeType enum to human-readable string
 * @param type - AST node type
 * @return     - String representation of the node type
 */
const char *ast_node_type_string(ASTNodeType type);

#endif // AST_H