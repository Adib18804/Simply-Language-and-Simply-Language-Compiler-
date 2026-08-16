#include "ast.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// AST NODE POOL MANAGEMENT
// ============================================================

// Static pool for storing all AST nodes
static ASTNode node_pool[MAX_AST_NODES];

// Current number of nodes allocated from the pool
static int node_pool_count = 0;

// ============================================================
// 1. CREATE A NEW AST NODE
// ============================================================
ASTNode *ast_create_node(ASTNodeType type, const char *value, int line)
{

    // Check if node pool has available space
    if (node_pool_count >= MAX_AST_NODES)
    {
        fprintf(stderr, "AST Error: Node pool is full! Cannot create more nodes.\n");
        return NULL;
    }

    // Allocate node from pool
    ASTNode *new_node = &node_pool[node_pool_count];
    node_pool_count++;

    // Set node type
    new_node->type = type;

    // Copy value if provided
    if (value != NULL)
    {
        strncpy(new_node->value, value, sizeof(new_node->value) - 1);
        new_node->value[sizeof(new_node->value) - 1] = '\0';
    }
    else
    {
        new_node->value[0] = '\0';
    }

    // Initialize operator type as empty
    new_node->op_type[0] = '\0';

    // Set source line number for error reporting
    new_node->line = line;

    // Initialize child count to zero
    new_node->children_count = 0;

    return new_node;
}

// ============================================================
// 2. ADD A CHILD NODE TO A PARENT NODE
// ============================================================
void ast_add_child(ASTNode *parent_node, ASTNode *child_node)
{

    // Validate both nodes are non-NULL
    if (parent_node == NULL || child_node == NULL)
    {
        return;
    }

    // Add child only if capacity allows
    if (parent_node->children_count < MAX_CHILDREN)
    {
        parent_node->children[parent_node->children_count] = child_node;
        parent_node->children_count++;
    }
    // Silently ignore if child limit exceeded
}

// ============================================================
// 3. PRINT AST TREE (FOR DEBUGGING)
// ============================================================
void ast_print(ASTNode *node, int depth)
{

    // Base case: NULL node
    if (node == NULL)
    {
        return;
    }

    // Print indentation based on tree depth
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }

    // Print node type
    printf("[%s", ast_node_type_string(node->type));

    // Print value if present
    if (node->value[0] != '\0')
    {
        printf(" '%s'", node->value);
    }

    // Print operator type if present
    if (node->op_type[0] != '\0')
    {
        printf(" op=%s", node->op_type);
    }

    // Print line number
    printf(" line=%d]\n", node->line);

    // Recursively print all children
    for (int i = 0; i < node->children_count; i++)
    {
        ast_print(node->children[i], depth + 1);
    }
}

// ============================================================
// 4. CONVERT NODE TYPE TO STRING
// ============================================================
const char *ast_node_type_string(ASTNodeType type)
{

    switch (type)
    {
    // Program structure
    case NODE_PROGRAM:
        return "PROGRAM";
    case NODE_STATEMENT_LIST:
        return "STATEMENT_LIST";

    // Declarations and assignments
    case NODE_DECLARATION:
        return "DECLARATION";
    case NODE_ASSIGNMENT:
        return "ASSIGNMENT";

    // I/O operations
    case NODE_OUTPUT:
        return "OUTPUT";
    case NODE_INPUT:
        return "INPUT";

    // Control flow
    case NODE_CONDITIONAL:
        return "CONDITIONAL";
    case NODE_LOOP:
        return "LOOP";

    // Task/Function declarations and calls
    case NODE_TASK_DECL:
        return "TASK_DECL";
    case NODE_TASK_CALL:
        return "TASK_CALL";
    case NODE_GIVE:
        return "GIVE";

    // Expressions
    case NODE_BINARY_OP:
        return "BINARY_OP";
    case NODE_UNARY_OP:
        return "UNARY_OP";

    // Literal values
    case NODE_INTEGER:
        return "INTEGER";
    case NODE_DECIMAL:
        return "DECIMAL";
    case NODE_STRING:
        return "STRING";
    case NODE_BOOLEAN:
        return "BOOLEAN";
    case NODE_IDENTIFIER:
        return "IDENTIFIER";

    // Fallback for unknown types
    default:
        return "UNKNOWN";
    }
}

// ============================================================
// 5. FREE THE ENTIRE AST (RESET POOL)
// ============================================================
void ast_free(ASTNode *root)
{
    // Reset pool counter to free all nodes
    node_pool_count = 0;

    // Suppress unused parameter warning
    (void)root;
}