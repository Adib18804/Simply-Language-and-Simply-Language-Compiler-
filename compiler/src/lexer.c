#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// ============================================================
// LEXER STATE
// ============================================================

static const char *source_code; // Input source code string
static int current_position;    // Current position in source code
static int current_line;        // Current line number
static int current_column;      // Current column number

// ============================================================
// KEYWORD TABLE
// ============================================================

/**
 * Keyword table mapping Simply language keywords to their token subtypes
 * Each entry: { keyword_string, subtype_string }
 */
static struct
{
    const char *name; // Keyword string (e.g., "make")
    const char *sub;  // Subtype (e.g., "MAKE")
} keywords[] = {
    {"make", "MAKE"},
    {"show", "SHOW"},
    {"ask", "ASK"},
    {"check", "CHECK"},
    {"otherwise", "OTHERWISE"},
    {"repeat", "REPEAT"},
    {"task", "TASK"},
    {"give", "GIVE"},
    {"finish", "FINISH"},
    {"true", "TRUE"},
    {"false", "FALSE"},
    {NULL, NULL} // Sentinel value to mark end of table
};

// ============================================================
// KEYWORD CHECKING
// ============================================================

/**
 * Check if a given word is a reserved keyword
 * @param word    - Word to check
 * @param sub_out - Buffer to store subtype (if keyword found)
 * @return        - 1 if keyword found, 0 otherwise
 */
static int is_keyword(const char *word, char *sub_out)
{
    for (int i = 0; keywords[i].name != NULL; i++)
    {
        if (strcmp(keywords[i].name, word) == 0)
        {
            strcpy(sub_out, keywords[i].sub);
            return 1;
        }
    }
    return 0;
}

// ============================================================
// LEXER INITIALIZATION
// ============================================================

/**
 * Initialize the lexer with source code
 * @param source - Input source code string
 */
void lexer_init(const char *source)
{
    source_code = source;
    current_position = 0;
    current_line = 1;
    current_column = 1;
}

// ============================================================
// CHARACTER HANDLING
// ============================================================

/**
 * Peek at the next character without consuming it
 * @return - Current character or '\0' if at end
 */
static char peek_char()
{
    return source_code[current_position];
}

/**
 * Consume and return the next character
 * Updates line and column tracking
 * @return - The consumed character
 */
static char advance_char()
{
    char c = source_code[current_position++];

    if (c == '\n')
    {
        current_line++;
        current_column = 1;
    }
    else
    {
        current_column++;
    }

    return c;
}

/**
 * Skip whitespace characters (space, tab, carriage return)
 */
static void skip_whitespace()
{
    while (peek_char() != '\0' &&
           (peek_char() == ' ' || peek_char() == '\t' || peek_char() == '\r'))
    {
        advance_char();
    }
}

/**
 * Skip comments starting with '#'
 * Comments extend until the end of line
 */
static void skip_comment()
{
    if (peek_char() == '#')
    {
        while (peek_char() != '\0' && peek_char() != '\n')
        {
            advance_char();
        }
    }
}

// ============================================================
// TOKEN CREATION
// ============================================================

/**
 * Create a new token with the given properties
 * @param type   - Token type
 * @param value  - Token value (lexeme)
 * @param sub    - Subtype (optional, can be NULL)
 * @param ln     - Line number
 * @param cl     - Column number
 * @return       - The created Token
 */
static Token make_token(TokenType type, const char *value, const char *sub, int ln, int cl)
{
    Token token;

    // Set token type
    token.type = type;

    // Copy value (lexeme)
    strncpy(token.value, value, MAX_TOKEN_LEN - 1);
    token.value[MAX_TOKEN_LEN - 1] = '\0';

    // Copy subtype if provided
    if (sub != NULL)
    {
        strncpy(token.sub_type, sub, MAX_TOKEN_LEN - 1);
    }
    else
    {
        token.sub_type[0] = '\0';
    }
    token.sub_type[MAX_TOKEN_LEN - 1] = '\0';

    // Set position information
    token.line = ln;
    token.col = cl;

    return token;
}

// ============================================================
// MAIN TOKENIZATION
// ============================================================

/**
 * Tokenize the entire input source code
 * @return - TokenList containing all tokens
 */
TokenList lexer_tokenize(void)
{
    TokenList token_list;
    token_list.count = 0;

    char buffer[MAX_TOKEN_LEN];

    // Process until end of input
    while (peek_char() != '\0')
    {

        // Skip whitespace and comments
        skip_whitespace();
        while (peek_char() == '#')
        {
            skip_comment();
            skip_whitespace();
        }

        // Exit if at end
        if (peek_char() == '\0')
        {
            break;
        }

        // Save position for token tracking
        int start_column = current_column;
        int start_line = current_line;
        char current_char = peek_char();

        // ============================================================
        // HANDLE NEWLINE
        // ============================================================
        if (current_char == '\n')
        {
            advance_char();

            // Avoid duplicate consecutive newlines
            if (token_list.count == 0 ||
                (token_list.count > 0 &&
                 token_list.tokens[token_list.count - 1].type != TOK_NEWLINE))
            {
                token_list.tokens[token_list.count++] =
                    make_token(TOK_NEWLINE, "\\n", "NEWLINE", start_line, start_column);
            }
            continue;
        }

        // ============================================================
        // HANDLE STRING LITERALS
        // ============================================================
        if (current_char == '"')
        {
            advance_char(); // Consume opening quote

            int buffer_index = 0;

            // Read characters until closing quote
            while (peek_char() != '\0' && peek_char() != '"')
            {
                // Handle escape sequences
                if (peek_char() == '\\' && source_code[current_position + 1] != '\0')
                {
                    advance_char(); // Consume backslash
                }

                // Store character if buffer has space
                if (buffer_index < MAX_TOKEN_LEN - 2)
                {
                    buffer[buffer_index++] = advance_char();
                }
                else
                {
                    advance_char(); // Skip if buffer is full
                }
            }

            // Consume closing quote if present
            if (peek_char() == '"')
            {
                advance_char();
            }

            buffer[buffer_index] = '\0';

            token_list.tokens[token_list.count++] =
                make_token(TOK_STRING, buffer, "STRING", start_line, start_column);
            continue;
        }

        // ============================================================
        // HANDLE NUMBERS (INTEGER AND DECIMAL)
        // ============================================================
        if (isdigit(current_char))
        {
            int buffer_index = 0;
            int is_decimal = 0;

            // Read integer part
            while (isdigit(peek_char()))
            {
                buffer[buffer_index++] = advance_char();
            }

            // Check for decimal point
            if (peek_char() == '.' && isdigit(source_code[current_position + 1]))
            {
                is_decimal = 1;
                buffer[buffer_index++] = advance_char(); // Consume '.'

                // Read fractional part
                while (isdigit(peek_char()))
                {
                    buffer[buffer_index++] = advance_char();
                }
            }

            buffer[buffer_index] = '\0';

            // Create appropriate token type
            if (is_decimal)
            {
                token_list.tokens[token_list.count++] =
                    make_token(TOK_DECIMAL, buffer, "DECIMAL", start_line, start_column);
            }
            else
            {
                token_list.tokens[token_list.count++] =
                    make_token(TOK_INTEGER, buffer, "INTEGER", start_line, start_column);
            }
            continue;
        }

        // ============================================================
        // HANDLE IDENTIFIERS AND KEYWORDS
        // ============================================================
        if (isalpha(current_char) || current_char == '_')
        {
            int buffer_index = 0;

            // Read alphanumeric characters and underscores
            while (isalnum(peek_char()) || peek_char() == '_')
            {
                buffer[buffer_index++] = advance_char();
            }
            buffer[buffer_index] = '\0';

            // Check if it's a keyword
            char subtype[128];
            if (is_keyword(buffer, subtype))
            {
                // Boolean literals are a special case
                if (strcmp(subtype, "TRUE") == 0 || strcmp(subtype, "FALSE") == 0)
                {
                    token_list.tokens[token_list.count++] =
                        make_token(TOK_BOOLEAN, buffer, subtype, start_line, start_column);
                }
                else
                {
                    token_list.tokens[token_list.count++] =
                        make_token(TOK_KEYWORD, buffer, subtype, start_line, start_column);
                }
            }
            else
            {
                // Regular identifier
                token_list.tokens[token_list.count++] =
                    make_token(TOK_IDENTIFIER, buffer, "IDENTIFIER", start_line, start_column);
            }
            continue;
        }

        // ============================================================
        // HANDLE TWO-CHARACTER OPERATORS
        // ============================================================
        // Equality (==)
        if (current_char == '=' && source_code[current_position + 1] == '=')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, "==", "EQ", start_line, start_column);
            continue;
        }

        // Inequality (!=)
        if (current_char == '!' && source_code[current_position + 1] == '=')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, "!=", "NEQ", start_line, start_column);
            continue;
        }

        // Less than or equal (<=)
        if (current_char == '<' && source_code[current_position + 1] == '=')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, "<=", "LTE", start_line, start_column);
            continue;
        }

        // Greater than or equal (>=)
        if (current_char == '>' && source_code[current_position + 1] == '=')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, ">=", "GTE", start_line, start_column);
            continue;
        }

        // Logical AND (&&)
        if (current_char == '&' && source_code[current_position + 1] == '&')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_LOGICAL, "&&", "AND", start_line, start_column);
            continue;
        }

        // Logical OR (||)
        if (current_char == '|' && source_code[current_position + 1] == '|')
        {
            advance_char();
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_LOGICAL, "||", "OR", start_line, start_column);
            continue;
        }

        // ============================================================
        // HANDLE SINGLE-CHARACTER TOKENS
        // ============================================================
        switch (current_char)
        {
        case '=':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_ASSIGN, "=", "ASSIGN", start_line, start_column);
            break;

        case '+':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_OPERATOR, "+", "PLUS", start_line, start_column);
            break;

        case '-':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_OPERATOR, "-", "MINUS", start_line, start_column);
            break;

        case '*':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_OPERATOR, "*", "MUL", start_line, start_column);
            break;

        case '/':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_OPERATOR, "/", "DIV", start_line, start_column);
            break;

        case '%':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_OPERATOR, "%", "MOD", start_line, start_column);
            break;

        case '<':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, "<", "LT", start_line, start_column);
            break;

        case '>':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_COMPARISON, ">", "GT", start_line, start_column);
            break;

        case '!':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_LOGICAL, "!", "NOT", start_line, start_column);
            break;

        case '(':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_LPAREN, "(", "LPAREN", start_line, start_column);
            break;

        case ')':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_RPAREN, ")", "RPAREN", start_line, start_column);
            break;

        case '{':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_LBRACE, "{", "LBRACE", start_line, start_column);
            break;

        case '}':
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_RBRACE, "}", "RBRACE", start_line, start_column);
            break;

        default:
            // Unknown character - report as error
            buffer[0] = current_char;
            buffer[1] = '\0';
            advance_char();
            token_list.tokens[token_list.count++] =
                make_token(TOK_ERROR, buffer, "INVALID", start_line, start_column);
            break;
        }
    }

    // Add end-of-file marker
    token_list.tokens[token_list.count++] =
        make_token(TOK_EOF, "EOF", "EOF", current_line, current_column);

    return token_list;
}

// ============================================================
// TOKEN TYPE TO STRING CONVERSION
// ============================================================

/**
 * Convert TokenType enum to a human-readable string
 * @param type - Token type
 * @return     - String representation
 */
const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOK_KEYWORD:
        return "KEYWORD";
    case TOK_IDENTIFIER:
        return "IDENTIFIER";
    case TOK_INTEGER:
        return "INTEGER";
    case TOK_DECIMAL:
        return "DECIMAL";
    case TOK_STRING:
        return "STRING";
    case TOK_BOOLEAN:
        return "BOOLEAN";
    case TOK_OPERATOR:
        return "OPERATOR";
    case TOK_COMPARISON:
        return "COMPARISON";
    case TOK_LOGICAL:
        return "LOGICAL";
    case TOK_ASSIGN:
        return "ASSIGNMENT";
    case TOK_LPAREN:
        return "LEFT_PAREN";
    case TOK_RPAREN:
        return "RIGHT_PAREN";
    case TOK_LBRACE:
        return "LEFT_BRACE";
    case TOK_RBRACE:
        return "RIGHT_BRACE";
    case TOK_NEWLINE:
        return "NEWLINE";
    case TOK_EOF:
        return "EOF";
    case TOK_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}