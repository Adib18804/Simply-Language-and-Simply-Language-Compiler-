#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "tac.h"

// ============================================================
// OPTIMIZER MODULE
// ============================================================

/**
 * Optimize a TAC (Three Address Code) program
 * 
 * Performs the following optimizations:
 *   - Constant Folding: Evaluate constant expressions at compile time
 *   - Constant Propagation: Replace variables with known constant values
 *   - Identity Elimination: Remove operations like x + 0, x * 1, x - 0
 *   - Zero Elimination: Remove operations like x * 0
 * 
 * @param input - Input TAC program to be optimized
 * @return      - Optimized TAC program with improved efficiency
 * 
 * Example:
 *   Input:
 *     t1 = 5 + 3
 *     t2 = t1 * 2
 *     x = t2 + 0
 * 
 *   Output:
 *     t1 = 8
 *     t2 = 16
 *     x = 16
 */
TACProgram optimizer_optimize(TACProgram* input);

#endif // OPTIMIZER_H