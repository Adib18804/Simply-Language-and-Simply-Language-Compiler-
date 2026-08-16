#include "optimizer.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// ============================================================
// CONSTANT DETECTION FUNCTIONS
// ============================================================

/**
 * Check if a string represents a numeric constant
 * (integer or decimal, optional minus sign)
 */
static int is_numeric_constant(const char *str)
{
    if (!str || !*str)
    {
        return 0;
    }

    // String literals are not numeric constants
    if (*str == '"')
    {
        return 0;
    }

    // Skip optional minus sign
    int i = (*str == '-') ? 1 : 0;

    // Must have at least one digit
    if (!str[i])
    {
        return 0;
    }

    // All characters must be digits or decimal point
    for (; str[i]; i++)
    {
        if (!isdigit(str[i]) && str[i] != '.')
        {
            return 0;
        }
    }

    return 1;
}

/**
 * Check if a string represents a boolean constant (true or false)
 */
static int is_boolean_constant(const char *str)
{
    return str && (!strcmp(str, "true") || !strcmp(str, "false"));
}

/**
 * Convert string to double
 */
static double string_to_number(const char *str)
{
    return atof(str);
}

// ============================================================
// CONSTANT FOLDING
// ============================================================

/**
 * Attempt to fold constants in a single TAC instruction
 * Performs compile-time evaluation of constant expressions
 *
 * @param instruction - TAC instruction to optimize
 * @return            - 1 if instruction was modified, 0 otherwise
 */
static int fold_constants_in_instruction(TACInstr *instruction)
{

    // Skip assignment instructions
    if (instruction->op == TAC_ASSIGN)
    {
        return 0;
    }

    // ============================================================
    // HANDLE BINARY OPERATIONS (ARITHMETIC AND COMPARISONS)
    // ============================================================
    if (instruction->op >= TAC_ADD && instruction->op <= TAC_GTE)
    {

        int is_arg1_constant = is_numeric_constant(instruction->arg1) ||
                               is_boolean_constant(instruction->arg1);
        int is_arg2_constant = is_numeric_constant(instruction->arg2) ||
                               is_boolean_constant(instruction->arg2);

        // Both operands must be constants to fold
        if (is_arg1_constant && is_arg2_constant)
        {

            // Convert operands to double
            double left = is_boolean_constant(instruction->arg1) ? (!strcmp(instruction->arg1, "true") ? 1.0 : 0.0) : string_to_number(instruction->arg1);

            double right = is_boolean_constant(instruction->arg2) ? (!strcmp(instruction->arg2, "true") ? 1.0 : 0.0) : string_to_number(instruction->arg2);

            double result = 0;

            // Perform the operation
            switch (instruction->op)
            {
            case TAC_ADD:
                result = left + right;
                break;
            case TAC_SUB:
                result = left - right;
                break;
            case TAC_MUL:
                result = left * right;
                break;
            case TAC_DIV:
                if (right != 0)
                    result = left / right;
                else
                    return 0;
                break;
            case TAC_MOD:
                if (right != 0)
                    result = fmod(left, right);
                else
                    return 0;
                break;
            case TAC_EQ:
                result = (left == right);
                break;
            case TAC_NEQ:
                result = (left != right);
                break;
            case TAC_LT:
                result = (left < right);
                break;
            case TAC_GT:
                result = (left > right);
                break;
            case TAC_LTE:
                result = (left <= right);
                break;
            case TAC_GTE:
                result = (left >= right);
                break;
            case TAC_AND:
                result = left && right;
                break;
            case TAC_OR:
                result = left || right;
                break;
            default:
                return 0;
            }

            // Store result as constant
            if (instruction->op >= TAC_EQ && instruction->op <= TAC_OR)
            {
                // Comparison results are boolean
                strcpy(instruction->arg1, result ? "true" : "false");
            }
            else if (result == (long long)result && fabs(result) < 1e15)
            {
                // Integer result
                sprintf(instruction->arg1, "%lld", (long long)result);
            }
            else
            {
                // Floating point result
                sprintf(instruction->arg1, "%.10g", result);
            }

            // Convert to assignment instruction
            instruction->op = TAC_ASSIGN;
            instruction->arg2[0] = '\0';

            return 1;
        }
    }

    // ============================================================
    // HANDLE UNARY NOT OPERATION
    // ============================================================
    if (instruction->op == TAC_NOT)
    {
        if (is_boolean_constant(instruction->arg1))
        {
            // Fold NOT: !true -> false, !false -> true
            strcpy(instruction->arg1,
                   !strcmp(instruction->arg1, "true") ? "false" : "true");
            instruction->op = TAC_ASSIGN;
            return 1;
        }
    }

    // ============================================================
    // HANDLE UNARY NEGATION
    // ============================================================
    if (instruction->op == TAC_NEG)
    {
        if (is_numeric_constant(instruction->arg1))
        {
            double value = string_to_number(instruction->arg1);
            value = -value;

            // Store result
            if (value == (long long)value)
            {
                sprintf(instruction->arg1, "%lld", (long long)value);
            }
            else
            {
                sprintf(instruction->arg1, "%.10g", value);
            }

            instruction->op = TAC_ASSIGN;
            return 1;
        }
    }

    // ============================================================
    // HANDLE IDENTITY OPERATIONS
    // ============================================================
    if (instruction->op == TAC_ADD ||
        instruction->op == TAC_SUB ||
        instruction->op == TAC_MUL)
    {

        int is_arg1_const = is_numeric_constant(instruction->arg1);
        int is_arg2_const = is_numeric_constant(instruction->arg2);

        // Multiplication by 1
        if (instruction->op == TAC_MUL)
        {
            if (is_arg1_const && string_to_number(instruction->arg1) == 1)
            {
                // x * 1 -> x
                strcpy(instruction->arg1, instruction->arg2);
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
            if (is_arg2_const && string_to_number(instruction->arg2) == 1)
            {
                // 1 * x -> x
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
            // Multiplication by 0
            if (is_arg1_const && string_to_number(instruction->arg1) == 0)
            {
                // x * 0 -> 0
                strcpy(instruction->arg1, "0");
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
            if (is_arg2_const && string_to_number(instruction->arg2) == 0)
            {
                // 0 * x -> 0
                strcpy(instruction->arg1, "0");
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
        }

        // Addition by 0
        if (instruction->op == TAC_ADD)
        {
            if (is_arg1_const && string_to_number(instruction->arg1) == 0)
            {
                // 0 + x -> x
                strcpy(instruction->arg1, instruction->arg2);
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
            if (is_arg2_const && string_to_number(instruction->arg2) == 0)
            {
                // x + 0 -> x
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
        }

        // Subtraction by 0
        if (instruction->op == TAC_SUB)
        {
            if (is_arg2_const && string_to_number(instruction->arg2) == 0)
            {
                // x - 0 -> x
                instruction->op = TAC_ASSIGN;
                instruction->arg2[0] = '\0';
                return 1;
            }
        }
    }

    return 0;
}

// ============================================================
// CONSTANT PROPAGATION
// ============================================================

/**
 * Propagate known constant values throughout the TAC program
 * Replaces variable references with their constant values
 */
static void propagate_constants(TACProgram *program)
{

    // Structure to track constant variables
    typedef struct
    {
        char name[128];  // Variable name
        char value[512]; // Constant value
        int active;      // Whether this constant is still valid
    } ConstantEntry;

    ConstantEntry constants[512];
    int constant_count = 0;

    // Scan through all instructions
    for (int i = 0; i < program->count; i++)
    {
        TACInstr *instruction = &program->instructions[i];

        // Clear constant table at labels (control flow boundaries)
        if (instruction->op == TAC_LABEL ||
            instruction->op == TAC_JUMP ||
            instruction->op == TAC_JUMP_IF_FALSE)
        {
            constant_count = 0;
            continue;
        }

        // ============================================================
        // REPLACE OPERANDS WITH KNOWN CONSTANTS
        // ============================================================
        for (int j = 0; j < constant_count; j++)
        {
            if (constants[j].active)
            {
                // Replace arg1 if it matches
                if (strcmp(instruction->arg1, constants[j].name) == 0)
                {
                    strcpy(instruction->arg1, constants[j].value);
                }

                // Replace arg2 if it matches
                if (instruction->arg2[0] &&
                    strcmp(instruction->arg2, constants[j].name) == 0)
                {
                    strcpy(instruction->arg2, constants[j].value);
                }
            }
        }

        // ============================================================
        // TRACK NEW CONSTANTS FROM ASSIGNMENTS
        // ============================================================
        if (instruction->op == TAC_ASSIGN)
        {
            int is_constant = is_numeric_constant(instruction->arg1) ||
                              is_boolean_constant(instruction->arg1) ||
                              (instruction->arg1[0] == '"');

            if (is_constant)
            {
                // Check if variable already exists in constant table
                int found = 0;
                for (int j = 0; j < constant_count; j++)
                {
                    if (strcmp(constants[j].name, instruction->result) == 0)
                    {
                        // Update existing constant
                        strcpy(constants[j].value, instruction->arg1);
                        constants[j].active = 1;
                        found = 1;
                        break;
                    }
                }

                // Add new constant if space available
                if (!found && constant_count < 512)
                {
                    strcpy(constants[constant_count].name, instruction->result);
                    strcpy(constants[constant_count].value, instruction->arg1);
                    constants[constant_count].active = 1;
                    constant_count++;
                }
            }
            else
            {
                // Variable assigned non-constant value - invalidate constant
                for (int j = 0; j < constant_count; j++)
                {
                    if (strcmp(constants[j].name, instruction->result) == 0)
                    {
                        constants[j].active = 0;
                    }
                }
            }
        }
    }
}

// ============================================================
// MAIN OPTIMIZATION ENTRY POINT
// ============================================================

/**
 * Optimize a TAC program by performing constant folding and propagation
 *
 * Optimization passes:
 *   1. Constant Propagation - Replace variables with constant values
 *   2. Constant Folding - Evaluate constant expressions at compile time
 *
 * @param input - Input TAC program
 * @return      - Optimized TAC program
 */
TACProgram optimizer_optimize(TACProgram *input)
{

    // Create a copy of the input program
    TACProgram output = *input;

    // Run multiple optimization passes
    for (int pass = 0; pass < 5; pass++)
    {

        // Phase 1: Propagate constants
        propagate_constants(&output);

        // Phase 2: Fold constants
        int any_changes = 0;
        for (int i = 0; i < output.count; i++)
        {
            if (fold_constants_in_instruction(&output.instructions[i]))
            {
                any_changes = 1;
            }
        }

        // Stop if no improvements were made
        if (!any_changes)
        {
            break;
        }
    }

    return output;
}