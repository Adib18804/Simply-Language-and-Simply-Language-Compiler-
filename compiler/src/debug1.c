#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

// ============================================================
// DEBUG / TEST DRIVER FOR COMPILER PIPELINE
// ============================================================

/**
 * Main entry point for debugging the compiler pipeline
 *
 * This program demonstrates the complete compilation pipeline:
 *   1. Lexical Analysis (Tokenization)
 *   2. Parsing (AST Construction)
 *   3. AST Display for Verification
 *
 * @param argc - Argument count (unused)
 * @param argv - Argument vector (unused)
 * @return     - 0 on successful execution
 */
int main(int argc, char **argv)
{

    // ============================================================
    // TEST INPUT SOURCE CODE
    // ============================================================

    // Simple test program: variable declaration and initialization
    const char *source_code = "make x = 10";

    // Display source code being tested
    printf("Source: %s\n", source_code);
    printf("\n");

    // ============================================================
    // PHASE 1: LEXICAL ANALYSIS (TOKENIZATION)
    // ============================================================

    printf("========== LEXICAL ANALYSIS ==========\n");

    // Initialize lexer with source code
    lexer_init(source_code);

    // Tokenize the source code
    TokenList token_list = lexer_tokenize();

    // Display tokenization results
    printf("Tokenization complete. Total tokens: %d\n", token_list.count);
    printf("\n");

    // Print all tokens with details
    for (int i = 0; i < token_list.count; i++)
    {
        Token *token = &token_list.tokens[i];

        printf("  Token [%d]: Type='%s'", i, token_type_to_string(token->type));
        printf(" Value='%s'", token->value);
        printf(" Line=%d Col=%d\n", token->line, token->col);
    }

    printf("\n");

    // ============================================================
    // PHASE 2: PARSING (AST CONSTRUCTION)
    // ============================================================

    printf("========== PARSING ==========\n");
    printf("Starting parser...\n");

    // Parse the token list to build AST
    ParseResult parse_result = parser_parse(&token_list);

    // Display parsing results
    printf("Parsing complete.\n");
    printf("  Success: %s\n", parse_result.success ? "YES" : "NO");
    printf("  Error count: %d\n", parse_result.error_count);
    printf("\n");

    // ============================================================
    // ERROR REPORTING
    // ============================================================

    // Display any parsing errors
    if (parse_result.error_count > 0)
    {
        printf("========== PARSING ERRORS ==========\n");

        for (int i = 0; i < parse_result.error_count; i++)
        {
            ParseError *error = &parse_result.errors[i];

            printf("  Error %d: Line=%d Col=%d\n", i + 1, error->line, error->col);
            printf("    Message: %s\n", error->message);
        }

        printf("\n");
    }

    // ============================================================
    // PHASE 3: AST DISPLAY
    // ============================================================

    // Display the Abstract Syntax Tree if parsing succeeded
    if (parse_result.ast != NULL)
    {
        printf("========== ABSTRACT SYNTAX TREE ==========\n");
        printf("AST Structure:\n");
        ast_print(parse_result.ast, 0);
        printf("\n");
    }
    else
    {
        printf("No AST generated (parsing failed).\n");
    }

    // ============================================================
    // CLEANUP AND EXIT
    // ============================================================

    printf("========== DEBUG COMPLETE ==========\n");

    // Return success
    return 0;
}