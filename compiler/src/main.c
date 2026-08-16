#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "tac.h"
#include "optimizer.h"
#include "codegen.h"

// ============================================================
// FILE OUTPUT FUNCTIONS
// ============================================================

/**
 * Write token list to file
 */
static void write_tokens_to_file(TokenList *token_list, const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (!file)
        return;

    for (int i = 0; i < token_list->count; i++)
    {
        Token *token = &token_list->tokens[i];
        fprintf(file, "%s\t%s\t%s\t%d\t%d\n",
                token_type_to_string(token->type),
                token->sub_type[0] ? token->sub_type : "-",
                token->value,
                token->line,
                token->col);
    }

    fclose(file);
}

/**
 * Recursively write AST node to file with indentation
 */
static void write_ast_node_to_file(FILE *file, ASTNode *node, int depth)
{
    if (!node)
        return;

    // Print indentation
    for (int i = 0; i < depth; i++)
    {
        fprintf(file, "  ");
    }

    // Print node type
    fprintf(file, "%s", ast_node_type_string(node->type));

    // Print value if present
    if (node->value[0])
    {
        fprintf(file, " | value=%s", node->value);
    }

    // Print operator type if present
    if (node->op_type[0])
    {
        fprintf(file, " | op=%s", node->op_type);
    }

    // Print line number
    fprintf(file, " | line=%d\n", node->line);

    // Recursively print children
    for (int i = 0; i < node->children_count; i++)
    {
        write_ast_node_to_file(file, node->children[i], depth + 1);
    }
}

/**
 * Write AST to file
 */
static void write_ast_to_file(ASTNode *ast, const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (!file)
        return;

    write_ast_node_to_file(file, ast, 0);

    fclose(file);
}

/**
 * Write semantic analysis results to file
 */
static void write_semantic_to_file(SemanticResult *semantic_result, const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (!file)
        return;

    // Write success status and error count
    fprintf(file, "SUCCESS=%d\n", semantic_result->success);
    fprintf(file, "ERRORS=%d\n", semantic_result->error_count);

    // Write all semantic errors
    for (int i = 0; i < semantic_result->error_count; i++)
    {
        SemanticError *error = &semantic_result->errors[i];
        fprintf(file, "ERR\t%s\t%d\t%s\n",
                error->error_type,
                error->line,
                error->message);
    }

    // Write symbol table
    fprintf(file, "---SYMBOLS---\n");
    for (int i = 0; i < semantic_result->symbol_table.count; i++)
    {
        Symbol *symbol = &semantic_result->symbol_table.symbols[i];
        fprintf(file, "SYM\t%s\t%s\t%d\t%d\n",
                symbol->name,
                symbol_type_to_string(symbol->type),
                symbol->scope_level,
                symbol->declared_line);
    }

    fclose(file);
}

/**
 * Write TAC (Three Address Code) program to file
 */
static void write_tac_to_file(TACProgram *tac_program, const char *file_path)
{
    FILE *file = fopen(file_path, "w");
    if (!file)
        return;

    for (int i = 0; i < tac_program->count; i++)
    {
        TACInstr *instruction = &tac_program->instructions[i];
        char op_str[32];

        // Convert TAC opcode to string
        switch (instruction->op)
        {
        case TAC_LABEL:
            strcpy(op_str, "LABEL");
            break;
        case TAC_JUMP:
            strcpy(op_str, "JUMP");
            break;
        case TAC_JUMP_IF_FALSE:
            strcpy(op_str, "JFALSE");
            break;
        case TAC_ASSIGN:
            strcpy(op_str, "ASSIGN");
            break;
        case TAC_ADD:
            strcpy(op_str, "ADD");
            break;
        case TAC_SUB:
            strcpy(op_str, "SUB");
            break;
        case TAC_MUL:
            strcpy(op_str, "MUL");
            break;
        case TAC_DIV:
            strcpy(op_str, "DIV");
            break;
        case TAC_MOD:
            strcpy(op_str, "MOD");
            break;
        case TAC_EQ:
            strcpy(op_str, "EQ");
            break;
        case TAC_NEQ:
            strcpy(op_str, "NEQ");
            break;
        case TAC_LT:
            strcpy(op_str, "LT");
            break;
        case TAC_GT:
            strcpy(op_str, "GT");
            break;
        case TAC_LTE:
            strcpy(op_str, "LTE");
            break;
        case TAC_GTE:
            strcpy(op_str, "GTE");
            break;
        case TAC_AND:
            strcpy(op_str, "AND");
            break;
        case TAC_OR:
            strcpy(op_str, "OR");
            break;
        case TAC_NOT:
            strcpy(op_str, "NOT");
            break;
        case TAC_NEG:
            strcpy(op_str, "NEG");
            break;
        case TAC_OUTPUT:
            strcpy(op_str, "OUTPUT");
            break;
        case TAC_INPUT:
            strcpy(op_str, "INPUT");
            break;
        case TAC_TASK_CALL:
            strcpy(op_str, "CALL");
            break;
        case TAC_RETURN:
            strcpy(op_str, "RETURN");
            break;
        default:
            strcpy(op_str, "?");
            break;
        }

        // Write instruction with all fields
        fprintf(file, "%d\t%s\t%s\t%s\t%s\t%d\n",
                i,                   // Instruction index
                op_str,              // Operation name
                instruction->result, // Result operand
                instruction->arg1,   // First argument
                instruction->arg2,   // Second argument
                instruction->line);  // Source line number
    }

    fclose(file);
}

// ============================================================
// FILE READING FUNCTION
// ============================================================

/**
 * Read entire file into buffer
 * @return - Number of bytes read, or 0 on failure
 */
static int read_file_contents(const char *file_path, char *output_buffer, int max_size)
{
    FILE *file = fopen(file_path, "r");
    if (!file)
        return 0;

    int bytes_read = fread(output_buffer, 1, max_size - 1, file);
    output_buffer[bytes_read] = '\0';

    fclose(file);
    return bytes_read;
}

// ============================================================
// STATUS FILE WRITER
// ============================================================

/**
 * Write status message to status.txt file
 */
static void write_status(const char *output_dir, const char *status_message)
{
    char file_path[1024];
    sprintf(file_path, "%s/status.txt", output_dir);

    FILE *file = fopen(file_path, "w");
    if (file)
    {
        fprintf(file, "%s\n", status_message);
        fclose(file);
    }
}

// ============================================================
// MAIN COMPILER ENTRY POINT
// ============================================================

/**
 * Main compiler driver
 *
 * Usage: ./compiler <input.simply> <output_dir>
 *
 * Compilation Pipeline:
 *   1. Read source file
 *   2. Lexical Analysis (Tokenization)
 *   3. Parsing (AST Construction)
 *   4. Semantic Analysis
 *   5. TAC Generation
 *   6. Optimization
 *   7. Code Generation (C code)
 *
 * Each phase writes its output to the specified output directory
 */
int main(int argc, char **argv)
{

    // ============================================================
    // COMMAND LINE ARGUMENT CHECKING
    // ============================================================

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <input.simply> <output_dir>\n", argv[0]);
        return 1;
    }

    const char *input_file_path = argv[1];
    const char *output_directory = argv[2];

    // ============================================================
    // PHASE 1: READ SOURCE FILE
    // ============================================================

    char source_code[65536];
    int source_length = read_file_contents(input_file_path, source_code, sizeof(source_code));

    if (source_length == 0)
    {
        fprintf(stderr, "ERROR: Cannot read input file %s\n", input_file_path);
        return 2;
    }

    char file_path[1024];

    // ============================================================
    // PHASE 2: LEXICAL ANALYSIS (TOKENIZATION)
    // ============================================================

    lexer_init(source_code);
    TokenList token_list = lexer_tokenize();

    // Write tokens to file
    sprintf(file_path, "%s/tokens.txt", output_directory);
    write_tokens_to_file(&token_list, file_path);

    // Check for lexical errors
    int has_lexical_error = 0;
    for (int i = 0; i < token_list.count; i++)
    {
        if (token_list.tokens[i].type == TOK_ERROR)
        {
            has_lexical_error = 1;
            break;
        }
    }

    if (has_lexical_error)
    {
        write_status(output_directory, "LEX_ERROR");
        return 3;
    }

    // ============================================================
    // PHASE 3: PARSING (AST CONSTRUCTION)
    // ============================================================

    ParseResult parse_result = parser_parse(&token_list);

    // Write AST to file
    sprintf(file_path, "%s/ast.txt", output_directory);
    write_ast_to_file(parse_result.ast, file_path);

    // Write parse errors to file
    sprintf(file_path, "%s/parse_errors.txt", output_directory);
    FILE *file = fopen(file_path, "w");
    if (file)
    {
        fprintf(file, "SUCCESS=%d\n", parse_result.success);
        fprintf(file, "ERRORS=%d\n", parse_result.error_count);

        for (int i = 0; i < parse_result.error_count; i++)
        {
            fprintf(file, "ERR\t%d\t%d\t%s\n",
                    parse_result.errors[i].line,
                    parse_result.errors[i].col,
                    parse_result.errors[i].message);
        }
        fclose(file);
    }

    // Check for parsing errors
    if (!parse_result.success || !parse_result.ast)
    {
        write_status(output_directory, "PARSE_ERROR");
        return 4;
    }

    // ============================================================
    // PHASE 4: SEMANTIC ANALYSIS
    // ============================================================

    SemanticResult semantic_result = semantic_analyze(parse_result.ast);

    // Write semantic analysis results
    sprintf(file_path, "%s/semantic.txt", output_directory);
    write_semantic_to_file(&semantic_result, file_path);

    // Check for semantic errors
    if (!semantic_result.success)
    {
        write_status(output_directory, "SEMANTIC_ERROR");
        return 5;
    }

    // ============================================================
    // PHASE 5: TAC GENERATION
    // ============================================================

    TACProgram tac_program = tac_generate(parse_result.ast);

    // Write TAC to file
    sprintf(file_path, "%s/tac.txt", output_directory);
    write_tac_to_file(&tac_program, file_path);

    // ============================================================
    // PHASE 6: OPTIMIZATION
    // ============================================================

    TACProgram optimized_tac = optimizer_optimize(&tac_program);

    // Write optimized TAC to file
    sprintf(file_path, "%s/optimized_tac.txt", output_directory);
    write_tac_to_file(&optimized_tac, file_path);

    // ============================================================
    // PHASE 7: CODE GENERATION
    // ============================================================

    CodeGenResult codegen_result = codegen_generate(&optimized_tac);

    // Write generated C code to file
    sprintf(file_path, "%s/output.c", output_directory);
    file = fopen(file_path, "w");
    if (file)
    {
        fputs(codegen_result.code, file);
        fclose(file);
    }

    // ============================================================
    // COMPILATION SUCCESS
    // ============================================================

    write_status(output_directory, "SUCCESS");

    return 0;
}