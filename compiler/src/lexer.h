#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================
// LEXER CONFIGURATION
// ============================================================

#define MAX_TOKEN_LEN 64 // Maximum length of a single token's value
#define MAX_TOKENS 512   // Maximum number of tokens in the token list

// ============================================================
// TOKEN TYPES
// ============================================================

/**
 * Enumeration of all possible token types in the Simply language
 */
typedef enum
{
    // Keywords (make, show, ask, check, otherwise, repeat, task, give, finish)
    TOK_KEYWORD,

    // Identifiers (variable names, function names)
    TOK_IDENTIFIER,

    // Literals
    TOK_INTEGER, // Integer literal (e.g., 42)
    TOK_DECIMAL, // Decimal literal (e.g., 3.14)
    TOK_STRING,  // String literal (e.g., "hello")
    TOK_BOOLEAN, // Boolean literal (true, false)

    // Operators
    TOK_OPERATOR,   // Arithmetic operators (+, -, *, /, %)
    TOK_COMPARISON, // Comparison operators (==, !=, <, >, <=, >=)
    TOK_LOGICAL,    // Logical operators (&&, ||, !)
    TOK_ASSIGN,     // Assignment operator (=)

    // Punctuation
    TOK_LPAREN, // Left parenthesis (
    TOK_RPAREN, // Right parenthesis )
    TOK_LBRACE, // Left brace {
    TOK_RBRACE, // Right brace }

    // Special tokens
    TOK_NEWLINE, // Newline character (statement terminator)
    TOK_EOF,     // End of file marker
    TOK_ERROR    // Error token (for invalid characters)
} TokenType;

// ============================================================
// TOKEN STRUCTURE
// ============================================================

/**
 * Structure representing a single token
 */
typedef struct
{
    TokenType type;               // Type of the token
    char value[MAX_TOKEN_LEN];    // Lexeme (actual text of the token)
    char sub_type[MAX_TOKEN_LEN]; // Subtype (e.g., "MAKE", "PLUS", "EQ", etc.)
    int line;                     // Line number in source code
    int col;                      // Column number in source code
} Token;

// ============================================================
// TOKEN LIST STRUCTURE
// ============================================================

/**
 * Structure containing a list of tokens
 */
typedef struct
{
    Token tokens[MAX_TOKENS]; // Array of tokens
    int count;                // Number of tokens in the list
} TokenList;

// ============================================================
// LEXER FUNCTIONS
// ============================================================

/**
 * Initialize the lexer with source code
 * @param source - Input source code string to tokenize
 */
void lexer_init(const char *source);

/**
 * Tokenize the entire source code
 * @return - TokenList containing all tokens
 */
TokenList lexer_tokenize(void);

/**
 * Convert TokenType enum to human-readable string
 * @param type - Token type
 * @return     - String representation of the token type
 */
const char *token_type_to_string(TokenType type);

#endif // LEXER_H