#include "parser.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// PARSER STATE
// ============================================================

static TokenList *token_list;    // Current token list being parsed
static int current_position;     // Current position in token list
static ParseResult parse_result; // Result of parsing (AST + errors)

// ============================================================
// TOKEN ACCESS FUNCTIONS
// ============================================================

/**
 * Peek at a token at a specific offset from current position
 * @param offset - Offset from current position (0 = current)
 * @return       - Pointer to the token
 */
static Token *peek_token(int offset)
{
    int position = current_position + offset;

    // If beyond end, return last token (EOF)
    if (position >= token_list->count)
    {
        return &token_list->tokens[token_list->count - 1];
    }

    return &token_list->tokens[position];
}

/**
 * Get the current token
 */
static Token *get_current_token()
{
    return peek_token(0);
}

/**
 * Consume the current token and advance
 */
static Token *consume_token()
{
    return &token_list->tokens[current_position++];
}

// ============================================================
// NEWLINE HANDLING
// ============================================================

/**
 * Skip all consecutive newline tokens
 */
static int skip_newlines()
{
    while (current_position < token_list->count &&
           token_list->tokens[current_position].type == TOK_NEWLINE)
    {
        current_position++;
    }
    return 1;
}

// ============================================================
// ERROR HANDLING
// ============================================================

/**
 * Add a parsing error to the result
 */
static void add_parse_error(const char *message, Token *token)
{
    if (parse_result.error_count >= 256)
    {
        return; // Maximum errors reached
    }

    ParseError *error = &parse_result.errors[parse_result.error_count++];
    snprintf(error->message, 1024, "%s", message);
    error->line = token ? token->line : 0;
    error->col = token ? token->col : 0;
    parse_result.success = 0;
}

// ============================================================
// MATCHING AND EXPECTING FUNCTIONS
// ============================================================

/**
 * Check if current token matches type and subtype
 */
static int match_token(TokenType type, const char *subtype)
{
    Token *token = get_current_token();
    return (token->type == type) && (strcmp(token->sub_type, subtype) == 0);
}

/**
 * Accept token if it matches (consume if matched)
 */
static int accept_token(TokenType type, const char *subtype)
{
    if (match_token(type, subtype))
    {
        consume_token();
        return 1;
    }
    return 0;
}

/**
 * Expect token and add error if not present
 */
static int expect_token(TokenType type, const char *subtype, const char *error_message)
{
    if (accept_token(type, subtype))
    {
        return 1;
    }
    add_parse_error(error_message, get_current_token());
    return 0;
}

// ============================================================
// FORWARD DECLARATIONS FOR EXPRESSION PARSING
// ============================================================

static ASTNode *parse_expression();
static ASTNode *parse_logical_or();
static ASTNode *parse_logical_and();
static ASTNode *parse_equality();
static ASTNode *parse_relational();
static ASTNode *parse_additive();
static ASTNode *parse_multiplicative();
static ASTNode *parse_unary();
static ASTNode *parse_primary();
static ASTNode *parse_statement();
static ASTNode *parse_statement_list();

// ============================================================
// STATEMENT LIST PARSING
// ============================================================

/**
 * Parse a list of statements
 * Used for program bodies, conditional bodies, loop bodies, task bodies
 */
static ASTNode *parse_statement_list()
{
    ASTNode *statement_list = ast_create_node(NODE_STATEMENT_LIST, "STMTS",
                                              get_current_token()->line);

    skip_newlines();

    while (current_position < token_list->count &&
           get_current_token()->type != TOK_EOF)
    {

        // Stop at FINISH or OTHERWISE (they terminate blocks)
        if (match_token(TOK_KEYWORD, "FINISH") ||
            match_token(TOK_KEYWORD, "OTHERWISE"))
        {
            break;
        }

        skip_newlines();

        if (get_current_token()->type == TOK_EOF)
        {
            break;
        }

        if (match_token(TOK_KEYWORD, "FINISH") ||
            match_token(TOK_KEYWORD, "OTHERWISE"))
        {
            break;
        }

        // Parse one statement
        ASTNode *statement = parse_statement();
        if (statement)
        {
            ast_add_child(statement_list, statement);
        }

        // Skip newlines after statement
        while (current_position < token_list->count &&
               token_list->tokens[current_position].type == TOK_NEWLINE)
        {
            current_position++;
        }
    }

    return statement_list;
}

// ============================================================
// STATEMENT PARSING
// ============================================================

/**
 * Parse a single statement
 * Handles: declaration, assignment, output, input, conditional, loop, task, give
 */
static ASTNode *parse_statement()
{
    Token *token = get_current_token();

    // Skip newlines
    if (token->type == TOK_NEWLINE)
    {
        consume_token();
        return NULL;
    }

    // ============================================================
    // DECLARATION: make x = 10
    // ============================================================
    if (match_token(TOK_KEYWORD, "MAKE"))
    {
        consume_token();

        ASTNode *declaration = ast_create_node(NODE_DECLARATION, "MAKE", token->line);

        // Parse identifier
        if (get_current_token()->type == TOK_IDENTIFIER)
        {
            ASTNode *identifier = ast_create_node(NODE_IDENTIFIER,
                                                  get_current_token()->value,
                                                  get_current_token()->line);
            ast_add_child(declaration, identifier);
            consume_token();
        }
        else
        {
            add_parse_error("Expected identifier after 'make'", get_current_token());
            return NULL;
        }

        // Expect '='
        if (!expect_token(TOK_ASSIGN, "ASSIGN", "Expected '=' in variable declaration"))
        {
            return NULL;
        }

        // Parse expression
        ASTNode *expression = parse_expression();
        if (expression)
        {
            ast_add_child(declaration, expression);
        }

        return declaration;
    }

    // ============================================================
    // OUTPUT: show expression
    // ============================================================
    if (match_token(TOK_KEYWORD, "SHOW"))
    {
        consume_token();

        ASTNode *output = ast_create_node(NODE_OUTPUT, "SHOW", token->line);

        ASTNode *expression = parse_expression();
        if (expression)
        {
            ast_add_child(output, expression);
        }

        return output;
    }

    // ============================================================
    // INPUT: ask "prompt" identifier
    // ============================================================
    if (match_token(TOK_KEYWORD, "ASK"))
    {
        consume_token();

        ASTNode *input = ast_create_node(NODE_INPUT, "ASK", token->line);

        // Parse prompt string
        if (get_current_token()->type == TOK_STRING)
        {
            ASTNode *prompt = ast_create_node(NODE_STRING,
                                              get_current_token()->value,
                                              get_current_token()->line);
            ast_add_child(input, prompt);
            consume_token();
        }
        else
        {
            add_parse_error("Expected prompt string after 'ask'", get_current_token());
            return NULL;
        }

        // Parse target identifier
        if (get_current_token()->type == TOK_IDENTIFIER)
        {
            ASTNode *identifier = ast_create_node(NODE_IDENTIFIER,
                                                  get_current_token()->value,
                                                  get_current_token()->line);
            ast_add_child(input, identifier);
            consume_token();
        }
        else
        {
            add_parse_error("Expected identifier for input target", get_current_token());
            return NULL;
        }

        return input;
    }

    // ============================================================
    // CONDITIONAL: check expression ... otherwise ... finish
    // ============================================================
    if (match_token(TOK_KEYWORD, "CHECK"))
    {
        consume_token();

        ASTNode *conditional = ast_create_node(NODE_CONDITIONAL, "CHECK", token->line);

        // Parse condition expression
        ASTNode *condition = parse_expression();
        if (condition)
        {
            ast_add_child(conditional, condition);
        }

        // Skip newlines before body
        while (current_position < token_list->count &&
               token_list->tokens[current_position].type == TOK_NEWLINE)
        {
            current_position++;
        }

        // Parse if-block
        ASTNode *if_block = parse_statement_list();
        ast_add_child(conditional, if_block);

        // Parse else-block if present
        ASTNode *else_block = ast_create_node(NODE_STATEMENT_LIST, "ELSE_STMTS",
                                              get_current_token()->line);
        if (match_token(TOK_KEYWORD, "OTHERWISE"))
        {
            consume_token();

            while (current_position < token_list->count &&
                   token_list->tokens[current_position].type == TOK_NEWLINE)
            {
                current_position++;
            }

            else_block = parse_statement_list();
        }
        ast_add_child(conditional, else_block);

        // Expect 'finish'
        if (!expect_token(TOK_KEYWORD, "FINISH", "Expected 'finish' to close conditional"))
        {
            return NULL;
        }

        return conditional;
    }

    // ============================================================
    // LOOP: repeat expression ... finish
    // ============================================================
    if (match_token(TOK_KEYWORD, "REPEAT"))
    {
        consume_token();

        ASTNode *loop = ast_create_node(NODE_LOOP, "REPEAT", token->line);

        // Parse repeat count expression
        ASTNode *count = parse_expression();
        if (count)
        {
            ast_add_child(loop, count);
        }

        // Skip newlines before body
        while (current_position < token_list->count &&
               token_list->tokens[current_position].type == TOK_NEWLINE)
        {
            current_position++;
        }

        // Parse loop body
        ASTNode *body = parse_statement_list();
        ast_add_child(loop, body);

        // Expect 'finish'
        if (!expect_token(TOK_KEYWORD, "FINISH", "Expected 'finish' to close loop"))
        {
            return NULL;
        }

        return loop;
    }

    // ============================================================
    // TASK DECLARATION: task name() ... finish
    // ============================================================
    if (match_token(TOK_KEYWORD, "TASK"))
    {
        consume_token();

        ASTNode *task_decl = ast_create_node(NODE_TASK_DECL, "TASK", token->line);

        // Parse task name
        if (get_current_token()->type == TOK_IDENTIFIER)
        {
            ASTNode *identifier = ast_create_node(NODE_IDENTIFIER,
                                                  get_current_token()->value,
                                                  get_current_token()->line);
            ast_add_child(task_decl, identifier);
            consume_token();
        }
        else
        {
            add_parse_error("Expected task name", get_current_token());
            return NULL;
        }

        // Parse parameter list ()
        if (!expect_token(TOK_LPAREN, "LPAREN", "Expected '('"))
        {
            return NULL;
        }
        if (!expect_token(TOK_RPAREN, "RPAREN", "Expected ')'"))
        {
            return NULL;
        }

        // Skip newlines before body
        while (current_position < token_list->count &&
               token_list->tokens[current_position].type == TOK_NEWLINE)
        {
            current_position++;
        }

        // Parse task body
        ASTNode *body = parse_statement_list();
        ast_add_child(task_decl, body);

        // Expect 'finish'
        if (!expect_token(TOK_KEYWORD, "FINISH", "Expected 'finish' to close task"))
        {
            return NULL;
        }

        return task_decl;
    }

    // ============================================================
    // GIVE STATEMENT: give expression
    // ============================================================
    if (match_token(TOK_KEYWORD, "GIVE"))
    {
        consume_token();

        ASTNode *give = ast_create_node(NODE_GIVE, "GIVE", token->line);

        ASTNode *expression = parse_expression();
        if (expression)
        {
            ast_add_child(give, expression);
        }

        return give;
    }

    // ============================================================
    // TASK CALL: identifier()
    // ============================================================
    if (get_current_token()->type == TOK_IDENTIFIER)
    {
        if (peek_token(1)->type == TOK_LPAREN)
        {
            ASTNode *task_call = ast_create_node(NODE_TASK_CALL,
                                                 get_current_token()->value,
                                                 get_current_token()->line);
            consume_token();

            expect_token(TOK_LPAREN, "LPAREN", "Expected '('");
            expect_token(TOK_RPAREN, "RPAREN", "Expected ')'");

            return task_call;
        }

        // ============================================================
        // ASSIGNMENT: identifier = expression
        // ============================================================
        Token *identifier_token = consume_token();

        if (get_current_token()->type == TOK_ASSIGN)
        {
            consume_token();

            ASTNode *assignment = ast_create_node(NODE_ASSIGNMENT,
                                                  identifier_token->value,
                                                  identifier_token->line);

            ASTNode *identifier = ast_create_node(NODE_IDENTIFIER,
                                                  identifier_token->value,
                                                  identifier_token->line);
            ast_add_child(assignment, identifier);

            ASTNode *expression = parse_expression();
            if (expression)
            {
                ast_add_child(assignment, expression);
            }

            return assignment;
        }
        else
        {
            add_parse_error("Expected '=' after identifier for assignment",
                            get_current_token());
            return NULL;
        }
    }

    // ============================================================
    // UNEXPECTED TOKEN
    // ============================================================
    char error_message[512];
    snprintf(error_message, 512, "Unexpected token: '%s'", token->value);
    add_parse_error(error_message, token);
    consume_token();

    return NULL;
}

// ============================================================
// EXPRESSION PARSING (PRECEDENCE HIERARCHY)
// ============================================================

/**
 * Expression -> Logical OR
 */
static ASTNode *parse_expression()
{
    return parse_logical_or();
}

/**
 * Logical OR -> Logical AND (||)
 */
static ASTNode *parse_logical_or()
{
    ASTNode *left = parse_logical_and();

    while (match_token(TOK_LOGICAL, "OR"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_logical_and();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, "||", left->line);
        strcpy(node->op_type, "OR");
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Logical AND -> Equality (&&)
 */
static ASTNode *parse_logical_and()
{
    ASTNode *left = parse_equality();

    while (match_token(TOK_LOGICAL, "AND"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_equality();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, "&&", left->line);
        strcpy(node->op_type, "AND");
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Equality -> Relational (==, !=)
 */
static ASTNode *parse_equality()
{
    ASTNode *left = parse_relational();

    while (match_token(TOK_COMPARISON, "EQ") ||
           match_token(TOK_COMPARISON, "NEQ"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_relational();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, operator_token->value, left->line);
        strcpy(node->op_type, operator_token->sub_type);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Relational -> Additive (<, >, <=, >=)
 */
static ASTNode *parse_relational()
{
    ASTNode *left = parse_additive();

    while (match_token(TOK_COMPARISON, "LT") ||
           match_token(TOK_COMPARISON, "GT") ||
           match_token(TOK_COMPARISON, "LTE") ||
           match_token(TOK_COMPARISON, "GTE"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_additive();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, operator_token->value, left->line);
        strcpy(node->op_type, operator_token->sub_type);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Additive -> Multiplicative (+, -)
 */
static ASTNode *parse_additive()
{
    ASTNode *left = parse_multiplicative();

    while (match_token(TOK_OPERATOR, "PLUS") ||
           match_token(TOK_OPERATOR, "MINUS"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_multiplicative();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, operator_token->value, left->line);
        strcpy(node->op_type, operator_token->sub_type);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Multiplicative -> Unary (*, /, %)
 */
static ASTNode *parse_multiplicative()
{
    ASTNode *left = parse_unary();

    while (match_token(TOK_OPERATOR, "MUL") ||
           match_token(TOK_OPERATOR, "DIV") ||
           match_token(TOK_OPERATOR, "MOD"))
    {
        Token *operator_token = consume_token();
        ASTNode *right = parse_unary();

        ASTNode *node = ast_create_node(NODE_BINARY_OP, operator_token->value, left->line);
        strcpy(node->op_type, operator_token->sub_type);
        ast_add_child(node, left);
        ast_add_child(node, right);
        left = node;
    }

    return left;
}

/**
 * Unary -> Primary (!, -)
 */
static ASTNode *parse_unary()
{
    // Logical NOT
    if (match_token(TOK_LOGICAL, "NOT"))
    {
        Token *operator_token = consume_token();
        ASTNode *operand = parse_unary();

        ASTNode *node = ast_create_node(NODE_UNARY_OP, "!", operator_token->line);
        strcpy(node->op_type, "NOT");
        ast_add_child(node, operand);

        return node;
    }

    // Unary minus
    if (match_token(TOK_OPERATOR, "MINUS"))
    {
        Token *operator_token = consume_token();
        ASTNode *operand = parse_unary();

        ASTNode *node = ast_create_node(NODE_UNARY_OP, "-", operator_token->line);
        strcpy(node->op_type, "NEG");
        ast_add_child(node, operand);

        return node;
    }

    return parse_primary();
}

/**
 * Primary -> Literal, Identifier, or Parenthesized Expression
 */
static ASTNode *parse_primary()
{
    Token *token = get_current_token();

    // Integer literal
    if (token->type == TOK_INTEGER)
    {
        consume_token();
        return ast_create_node(NODE_INTEGER, token->value, token->line);
    }

    // Decimal literal
    if (token->type == TOK_DECIMAL)
    {
        consume_token();
        return ast_create_node(NODE_DECIMAL, token->value, token->line);
    }

    // String literal
    if (token->type == TOK_STRING)
    {
        consume_token();
        return ast_create_node(NODE_STRING, token->value, token->line);
    }

    // Boolean literal
    if (token->type == TOK_BOOLEAN)
    {
        consume_token();
        return ast_create_node(NODE_BOOLEAN, token->value, token->line);
    }

    // Identifier
    if (token->type == TOK_IDENTIFIER)
    {
        consume_token();
        return ast_create_node(NODE_IDENTIFIER, token->value, token->line);
    }

    // Parenthesized expression: (expression)
    if (token->type == TOK_LPAREN)
    {
        consume_token();
        ASTNode *expression = parse_expression();

        if (!expect_token(TOK_RPAREN, "RPAREN", "Expected ')'"))
        {
            return expression;
        }

        return expression;
    }

    // Unexpected token
    char error_message[512];
    snprintf(error_message, 512, "Unexpected token in expression: '%s'", token->value);
    add_parse_error(error_message, token);
    consume_token();

    return NULL;
}

// ============================================================
// MAIN PARSER ENTRY POINT
// ============================================================

/**
 * Parse a token list and produce an Abstract Syntax Tree (AST)
 *
 * @param tokens - TokenList from lexer
 * @return       - ParseResult containing AST and any errors
 */
ParseResult parser_parse(TokenList *tokens)
{
    token_list = tokens;
    current_position = 0;
    parse_result.success = 1;
    parse_result.error_count = 0;
    parse_result.ast = NULL;

    // Create root program node
    ASTNode *program = ast_create_node(NODE_PROGRAM, "PROGRAM", 1);

    // Parse statement list
    ASTNode *statements = parse_statement_list();
    ast_add_child(program, statements);

    // Check for unexpected tokens after parsing
    if (current_position < token_list->count &&
        get_current_token()->type != TOK_EOF &&
        get_current_token()->type != TOK_NEWLINE)
    {

        if (parse_result.error_count == 0)
        {
            char error_message[512];
            snprintf(error_message, 512, "Unexpected tokens at end of input: '%s'",
                     get_current_token()->value);
            add_parse_error(error_message, get_current_token());
        }
    }

    parse_result.ast = program;

    return parse_result;
}