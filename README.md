# Simply — A Web-Based Compiler for a Custom Programming Language

**Course:** Compiler Design Lab  
**Project Title:** Simply: A Web-Based Educational Compiler  

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [The Simply Programming Language](#3-the-simply-programming-language)
4. [Keywords Reference](#4-keywords-reference)
5. [Operators Reference](#5-operators-reference)
6. [Compiler Pipeline — All Six Phases](#6-compiler-pipeline--all-six-phases)
   - [Phase 1: Lexical Analysis](#phase-1-lexical-analysis)
   - [Phase 2: Syntax Analysis](#phase-2-syntax-analysis)
   - [Phase 3: Semantic Analysis](#phase-3-semantic-analysis)
   - [Phase 4: Intermediate Code Generation](#phase-4-intermediate-code-generation-tac)
   - [Phase 5: Code Optimization](#phase-5-code-optimization)
   - [Phase 6: Target Code Generation](#phase-6-target-code-generation)
7. [Grammar Specification](#7-grammar-specification)
8. [Web Interface](#8-web-interface)
9. [Project Structure](#9-project-structure)
10. [How to Build and Run](#10-how-to-build-and-run)
11. [Example Programs](#11-example-programs)
12. [Technology Stack](#12-technology-stack)

---

## 1. Project Overview

**Simply** is a complete, working compiler built from scratch as a Compiler Design Lab project. It defines a small, readable custom programming language — also called Simply — and implements the full compilation pipeline in the C programming language.

The goal is to make every internal phase of compilation visible and understandable. Instead of hiding the process inside a black box, Simply exposes each stage — from the raw token list produced by the lexer, through the AST, semantic checks, intermediate code, optimized code, and final generated C — in a browser-based web interface.

### Compilation Flow

```
Simply Source Code
       │
       ▼
  Lexical Analysis  (lexer.c)      ──► tokens.txt
       │
       ▼
  Syntax Analysis   (parser.c)     ──► ast.txt, parse_errors.txt
       │
       ▼
  Semantic Analysis (semantic.c)   ──► semantic.txt (symbol table + errors)
       │
       ▼
  IR Generation     (tac.c)        ──► tac.txt
       │
       ▼
  Optimization      (optimizer.c)  ──► optimized_tac.txt
       │
       ▼
  Code Generation   (codegen.c)    ──► output.c
       │
       ▼
     GCC Compiler
       │
       ▼
  Executable + Program Output
```

---

## 2. System Architecture

The project is divided into three parts:

| Part | Technology | Role |
|---|---|---|
| Compiler (core) | C (GCC) | Processes Simply source through all 6 phases, writes output files |
| Backend (server) | Node.js + Express | Receives source from browser, invokes the compiler, returns JSON results |
| Frontend (browser) | HTML + CSS + JavaScript | Code editor, pipeline visualizer, tabbed output viewer |

### How they connect

1. The user writes Simply code in the browser editor.
2. Clicking **Compile** or **Run** sends the code to the Node.js backend via a POST request to `/api/compile`.
3. The backend writes the source to a temporary directory, then runs `simply-compiler.exe` with that file as input.
4. The compiler writes output files (`tokens.txt`, `ast.txt`, `semantic.txt`, `tac.txt`, `optimized_tac.txt`, `output.c`, `status.txt`) into that same directory.
5. The backend reads and parses all output files, then (if running) compiles `output.c` with GCC and executes the result.
6. Everything is returned as a single JSON response to the browser, which renders it across 8 tabs.

---

## 3. The Simply Programming Language

Simply is designed to be as readable as plain English. Every construct uses a clear keyword — no braces, no semicolons, no type annotations. Indentation is for human readability only; statements are separated by newlines.

### Data Types

| Type | Description | Example |
|---|---|---|
| `INTEGER` | Whole numbers | `42`, `-7`, `0` |
| `DECIMAL` | Floating-point numbers | `3.14`, `0.5` |
| `STRING` | Text in double quotes | `"Hello, World!"` |
| `BOOLEAN` | Logical value | `true`, `false` |

### Variable Declaration

Variables must be declared with `make` before use.

```
make x = 10
make name = "Alice"
make price = 9.99
make flag = true
```

### Assignment (after declaration)

```
make score = 0
score = score + 10
```

### Output

```
show x
show "Hello, World!"
show x + y
```

### Input

```
ask "Enter your name: " name
show name
```

### Conditional

```
check age >= 18
    show "Adult"
otherwise
    show "Minor"
finish
```

The `otherwise` block is optional:

```
check x > 0
    show "Positive"
finish
```

### Loop

`repeat` runs its body N times, counting down internally.

```
repeat 5
    show "hello"
finish
```

### Task (Function)

```
task greet()
    show "Hello from task!"
finish

greet()
```

---

## 4. Keywords Reference

| Keyword | Role | Example |
|---|---|---|
| `make` | Declare a variable | `make x = 10` |
| `show` | Print a value or expression | `show x + y` |
| `ask` | Read input from the user | `ask "Name: " name` |
| `check` | Start a conditional block | `check x > 0` |
| `otherwise` | Else branch of a conditional | `otherwise` |
| `repeat` | Loop N times | `repeat 10` |
| `finish` | Close a `check`, `repeat`, or `task` block | `finish` |
| `task` | Declare a named function | `task greet()` |
| `give` | Return a value from a task | `give result` |
| `true` | Boolean literal true | `make flag = true` |
| `false` | Boolean literal false | `make done = false` |

---

## 5. Operators Reference

### Arithmetic Operators

| Operator | Meaning | Example |
|---|---|---|
| `+` | Addition | `x + y` |
| `-` | Subtraction | `x - y` |
| `*` | Multiplication | `x * y` |
| `/` | Division | `x / y` |
| `%` | Modulo (remainder) | `x % 3` |

### Comparison Operators

| Operator | Meaning | Example |
|---|---|---|
| `==` | Equal to | `x == 10` |
| `!=` | Not equal to | `x != 0` |
| `<` | Less than | `age < 18` |
| `>` | Greater than | `score > 50` |
| `<=` | Less than or equal | `x <= 100` |
| `>=` | Greater than or equal | `age >= 18` |

### Logical Operators

| Operator | Meaning | Example |
|---|---|---|
| `&&` | Logical AND | `x > 0 && x < 10` |
| `\|\|` | Logical OR | `x == 0 \|\| x == 1` |
| `!` | Logical NOT | `!flag` |

### Operator Precedence (Lowest to Highest)

| Level | Operators |
|---|---|
| 1 (lowest) | `\|\|` |
| 2 | `&&` |
| 3 | `==`, `!=` |
| 4 | `<`, `>`, `<=`, `>=` |
| 5 | `+`, `-` |
| 6 | `*`, `/`, `%` |
| 7 (highest) | `!`, unary `-`, `( )` |

---

## 6. Compiler Pipeline — All Six Phases

### Phase 1: Lexical Analysis

**Source file:** `compiler/src/lexer.c` and `lexer.h`

The lexer (also called a scanner or tokenizer) reads the raw source code character by character and breaks it into a flat sequence of tokens. Each token has:

- **Type** — the category (KEYWORD, IDENTIFIER, INTEGER, STRING, OPERATOR, etc.)
- **Sub-type** — the specific kind within the category (MAKE, SHOW, PLUS, LT, etc.)
- **Value** — the actual text from the source
- **Line and Column** — position in the source for error reporting

**Token types produced:**

| Token Type | Sub-types |
|---|---|
| `KEYWORD` | MAKE, SHOW, ASK, CHECK, OTHERWISE, REPEAT, TASK, GIVE, FINISH |
| `BOOLEAN` | TRUE, FALSE |
| `IDENTIFIER` | (variable and task names) |
| `INTEGER` | (whole number literals) |
| `DECIMAL` | (floating-point literals) |
| `STRING` | (text in double quotes) |
| `OPERATOR` | PLUS, MINUS, MUL, DIV, MOD |
| `COMPARISON` | EQ, NEQ, LT, GT, LTE, GTE |
| `LOGICAL` | AND, OR, NOT |
| `ASSIGNMENT` | ASSIGN |
| `NEWLINE` | (statement terminator) |
| `EOF` | (end of input) |
| `ERROR` | (unrecognized character) |

**Key implementation details:**
- Whitespace (spaces, tabs) is skipped between tokens on the same line.
- Newlines are significant — they act as statement terminators and produce a `NEWLINE` token.
- Consecutive newlines produce only one `NEWLINE` token.
- Comments begin with `#` and extend to the end of the line.
- String literals are enclosed in double quotes (`"..."`).
- Multi-character operators (`==`, `!=`, `<=`, `>=`, `&&`, `||`) are handled by peeking one character ahead.

**Output:** `tokens.txt` — a tab-separated file with one token per line.

---

### Phase 2: Syntax Analysis

**Source file:** `compiler/src/parser.c` and `parser.h`

The parser takes the token list from the lexer and checks whether it conforms to the grammar of Simply. It builds an **Abstract Syntax Tree (AST)** — a tree where each node represents a language construct (declaration, assignment, conditional, loop, expression, etc.).

The parser uses **Recursive Descent Parsing** — a top-down parsing technique where each grammar rule is implemented as a function that calls other functions.

**Left recursion elimination** — The expression grammar was originally left-recursive (e.g., `expr → expr + term`). Left recursion cannot be handled by recursive descent. It was transformed to right-recursive form using prime rules:

```
Original (left-recursive):
  additive_expr → additive_expr + multiplicative_expr
                | multiplicative_expr

Transformed (right-recursive):
  additive_expr  → multiplicative_expr additive_prime
  additive_prime → + multiplicative_expr additive_prime
                 | ε
```

This transformation was applied at all levels of the expression hierarchy.

**AST Node Types:**

| Node Type | Represents |
|---|---|
| `PROGRAM` | Root of the entire program |
| `STATEMENT_LIST` | A sequence of statements |
| `DECLARATION` | `make x = expr` |
| `ASSIGNMENT` | `x = expr` |
| `OUTPUT` | `show expr` |
| `INPUT` | `ask "prompt" var` |
| `CONDITIONAL` | `check ... otherwise ... finish` |
| `LOOP` | `repeat N ... finish` |
| `TASK_DECL` | `task name() ... finish` |
| `TASK_CALL` | `name()` |
| `GIVE` | `give expr` |
| `BINARY_OP` | `a + b`, `a > b`, `a && b` |
| `UNARY_OP` | `!x`, `-x` |
| `IDENTIFIER` | Variable name |
| `INTEGER` | Integer literal |
| `DECIMAL` | Decimal literal |
| `STRING` | String literal |
| `BOOLEAN` | `true` or `false` |

**Error handling:** If the parser finds a token it does not expect, it records a `ParseError` with a message, line, and column, and attempts to continue parsing the rest of the file.

**Output:** `ast.txt` — a text-formatted tree, indented by depth. `parse_errors.txt` — list of any syntax errors found.

---

### Phase 3: Semantic Analysis

**Source file:** `compiler/src/semantic.c` and `semantic.h`, `symbol_table.c` and `symbol_table.h`

Semantic analysis traverses the AST and checks for meaning-level correctness — things the grammar cannot express. It builds and maintains a **symbol table** to track all declared variables, their types, and their scope levels.

**Checks performed:**

| Check | Error Type | Example |
|---|---|---|
| Variable used before declaration | `UNDECLARED_VARIABLE` | `show x` without `make x = ...` first |
| Variable declared twice in same scope | `DUPLICATE_DECLARATION` | `make x = 1` then `make x = 2` |
| Type mismatch on assignment | `TYPE_MISMATCH` | `make x = 5` then `x = "hello"` |
| Invalid operation on strings | `INVALID_OPERATION` | `make s = "hi" * 2` |

**Scope handling:** `check`, `repeat`, and `task` blocks each create a new scope. Variables declared inside a block are local to that block. When the block ends, the scope is exited and those variables go out of visibility.

**Type inference:** The analyzer infers the type of each expression from the AST without requiring explicit type annotations from the programmer. For example, `make x = 5` infers `x` as `INT`; `make name = "Alice"` infers `STRING`.

**Symbol table fields:** name, inferred type, scope level, declared line number.

**Output:** `semantic.txt` — contains a `SUCCESS/ERRORS` header, then any `ERR` lines, then a `---SYMBOLS---` section with one `SYM` line per declared variable.

---

### Phase 4: Intermediate Code Generation (TAC)

**Source file:** `compiler/src/tac.c` and `tac.h`

After semantic analysis confirms the program is correct, the TAC generator walks the AST and produces **Three-Address Code (TAC)** — a linear sequence of simple instructions where each instruction has at most three operands: a result and up to two arguments.

TAC is language-independent and machine-independent. It is a standard intermediate representation (IR) used in real compilers (similar to LLVM IR).

**TAC Instruction Format:**

```
result = arg1  op  arg2
```

**TAC Operations:**

| Operation | Notation | Meaning |
|---|---|---|
| `ASSIGN` | `x = y` | Copy value |
| `ADD` | `t0 = x + y` | Addition |
| `SUB` | `t0 = x - y` | Subtraction |
| `MUL` | `t0 = x * y` | Multiplication |
| `DIV` | `t0 = x / y` | Division |
| `MOD` | `t0 = x % y` | Modulo |
| `EQ` | `t0 = x == y` | Equality |
| `NEQ` | `t0 = x != y` | Inequality |
| `LT` | `t0 = x < y` | Less than |
| `GT` | `t0 = x > y` | Greater than |
| `LTE` | `t0 = x <= y` | Less than or equal |
| `GTE` | `t0 = x >= y` | Greater than or equal |
| `AND` | `t0 = x && y` | Logical AND |
| `OR` | `t0 = x \|\| y` | Logical OR |
| `NOT` | `t0 = !x` | Logical NOT |
| `NEG` | `t0 = -x` | Unary negation |
| `OUTPUT` | `show x` | Print value |
| `INPUT` | `x = ask "prompt"` | Read input |
| `LABEL` | `L0:` | Jump target |
| `JUMP` | `goto L1` | Unconditional jump |
| `JFALSE` | `if (!cond) goto L2` | Conditional jump |
| `CALL` | `call taskName` | Call a task |
| `RETURN` | `return x` | Return from task |

**Temporary variables** are generated automatically as `t0`, `t1`, `t2`, ... to hold intermediate expression results.

**Control flow** for `check`/`otherwise`/`finish` is compiled to labels and conditional jumps:

```
Simply:                          TAC:
check x > 0                        t0 = x > 0
    show "Positive"                if (!t0) goto L0
otherwise                          show "Positive"
    show "Negative"                goto L1
finish                           L0:
                                   show "Negative"
                                 L1:
```

**Loop** (`repeat N`) is compiled using a countdown counter (`__rc`):

```
Simply:                          TAC:
repeat 3                           __rc = 3
    show i                       L0:
finish                             t0 = 0 < __rc
                                   if (!t0) goto L1
                                   show i
                                   t1 = __rc - 1
                                   __rc = t1
                                   goto L0
                                 L1:
```

**Output:** `tac.txt` — tab-separated file with one instruction per line.

---

### Phase 5: Code Optimization

**Source file:** `compiler/src/optimizer.c` and `optimizer.h`

The optimizer takes the TAC program and applies transformations to make the generated code more efficient, without changing the program's behavior.

**Optimization passes run up to 5 times until no more changes occur:**

#### Constant Propagation

If a variable is assigned a constant value and not reassigned, all subsequent uses of that variable are replaced with the constant directly.

```
Before:           After:
x = 10            x = 10
t0 = x + 5        t0 = 10 + 5   ← x replaced with 10
```

#### Constant Folding

If both operands of an arithmetic or logical expression are constants, the result is computed at compile time and replaced with a single constant.

```
Before:           After:
t0 = 10 + 5       t0 = 15       ← computed at compile time
t1 = t0 * 2       t1 = 30
```

#### Algebraic Simplification (Strength Reduction)

Simple algebraic identities are applied:

| Pattern | Simplified To |
|---|---|
| `x * 1` | `x` |
| `x * 0` | `0` |
| `x + 0` | `x` |
| `x - 0` | `x` |

#### Boolean Constant Folding

`!true` → `false`, `!false` → `true`, and boolean expressions with constant operands are evaluated at compile time.

**Output:** `optimized_tac.txt` — same format as `tac.txt` but with optimizations applied.

---

### Phase 6: Target Code Generation

**Source file:** `compiler/src/codegen.c` and `codegen.h`

The code generator translates the optimized TAC into valid C source code. The generated C is then compiled by GCC to produce a native executable.

**Type inference:** The code generator scans all TAC instructions to infer the C type of each variable:
- Variables assigned a string literal → `char[4096]`
- Variables assigned a boolean or integer → `int`
- Temporary variables and others → `double`

**Generated C structure:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    // Variable declarations (inferred types)
    int x = 0;
    double t0 = 0;
    char name[4096];

    // TAC instructions translated to C statements
    x = 10;
    t0 = x + 5;
    printf("%g\n", (double)(t0));

    return 0;
}
```

**Output mapping:**

| TAC Operation | Generated C |
|---|---|
| `x = 5` | `x = 5;` |
| `strcpy` for strings | `strcpy(name, "Alice");` |
| `show x` (int/double) | `printf("%g\n", (double)(x));` |
| `show "text"` | `printf("%s\n", "text");` |
| `goto L0` | `goto L0;` |
| `if (!cond) goto L1` | `if (!(cond)) goto L1;` |
| `L0:` | `L0:` |
| `ask "prompt" x` | `printf("prompt"); fgets(x, 4096, stdin);` |

The final compiled executable is run by the backend, and its standard output is captured and sent back to the browser.

---

## 7. Grammar Specification

The complete formal grammar of Simply (after left-recursion elimination):

```
program            → statement_list

statement_list     → statement statement_list | ε

statement          → declaration
                   | assignment
                   | output_stmt
                   | input_stmt
                   | conditional
                   | loop_stmt
                   | task_decl
                   | task_call

declaration        → MAKE IDENTIFIER = expression
assignment         → IDENTIFIER = expression
output_stmt        → SHOW expression
input_stmt         → ASK STRING IDENTIFIER

conditional        → CHECK expression NEWLINE statement_list
                       OTHERWISE NEWLINE statement_list FINISH
                   | CHECK expression NEWLINE statement_list FINISH

loop_stmt          → REPEAT expression NEWLINE statement_list FINISH

task_decl          → TASK IDENTIFIER () NEWLINE statement_list FINISH
task_call          → IDENTIFIER ()
give_stmt          → GIVE expression

expression         → logical_or_expr

logical_or_expr    → logical_and_expr logical_or_prime
logical_or_prime   → || logical_and_expr logical_or_prime | ε

logical_and_expr   → equality_expr logical_and_prime
logical_and_prime  → && equality_expr logical_and_prime | ε

equality_expr      → relational_expr equality_prime
equality_prime     → (== | !=) relational_expr equality_prime | ε

relational_expr    → additive_expr relational_prime
relational_prime   → (< | > | <= | >=) additive_expr relational_prime | ε

additive_expr      → multiplicative_expr additive_prime
additive_prime     → (+ | -) multiplicative_expr additive_prime | ε

multiplicative_expr → unary_expr multiplicative_prime
multiplicative_prime → (* | / | %) unary_expr multiplicative_prime | ε

unary_expr         → ! unary_expr | - unary_expr | primary_expr

primary_expr       → INTEGER | DECIMAL | STRING | BOOLEAN
                   | IDENTIFIER | ( expression )
```

---

## 8. Web Interface

The web interface is served by the Node.js backend and runs entirely in the browser.

### Header

- **Brand** — Project name and logo
- **Pipeline bar** — Seven steps (Lexer, Parser, Semantic, IR/TAC, Optimize, Codegen, Execute) that light up with colour as each phase completes
- **Example selector** — Dropdown to load built-in example programs
- **Clear** — Reset the editor and all output
- **Compile** — Run all compiler phases without executing the output
- **Run** — Compile and execute the program, showing the result

### Status Bar

Shows current language mode, cursor position (line and column), token count, and keyboard shortcut hints.

### Editor Pane (left)

- CodeMirror editor with line numbers, active-line highlight, and bracket matching
- Quick Reference panel showing the most-used Simply syntax at a glance

### Output Pane (right) — 8 Tabs

| Tab | Content |
|---|---|
| **Tokens** | Table of all tokens: type, sub-type, value, line, column |
| **AST** | Full Abstract Syntax Tree in indented text format |
| **Semantic** | Pass/fail status, symbol table (name, type, scope, line), semantic error messages |
| **TAC** | Three-Address Code instructions in readable format |
| **Optimized** | Optimized TAC after constant folding and propagation |
| **Generated C** | The C source code that was sent to GCC |
| **Output** | Program execution result in a terminal-style window |
| **Errors** | All errors from all phases (lexical, syntax, semantic, runtime) with locations |

### Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl + Enter` | Compile |
| `Ctrl + Shift + Enter` | Compile and Run |
| `Ctrl + L` | Clear editor |

---

## 9. Project Structure

```
simply-compiler/
│
├── compiler/                    ← C compiler (core)
│   ├── src/
│   │   ├── main.c               ← Entry point: orchestrates all phases
│   │   ├── lexer.c / lexer.h    ← Phase 1: Lexical Analysis
│   │   ├── parser.c / parser.h  ← Phase 2: Syntax Analysis
│   │   ├── ast.c / ast.h        ← AST node definitions and utilities
│   │   ├── semantic.c / .h      ← Phase 3: Semantic Analysis
│   │   ├── symbol_table.c / .h  ← Symbol table (used by semantic)
│   │   ├── tac.c / tac.h        ← Phase 4: TAC Generation
│   │   ├── optimizer.c / .h     ← Phase 5: Optimization
│   │   └── codegen.c / codegen.h← Phase 6: C Code Generation
│   ├── grammar/
│   │   └── simply_grammar.txt   ← Formal grammar specification
│   └── simply-compiler.exe      ← Built compiler binary (Windows)
│
├── backend/                     ← Node.js server
│   ├── server.js                ← Express app, static file serving
│   ├── routes/
│   │   └── compile.js           ← POST /api/compile endpoint
│   ├── services/
│   │   └── compilerService.js   ← Invokes compiler, parses outputs, runs GCC
│   └── temp/                    ← Temporary per-session compile directories
│
├── frontend/                    ← Browser UI
│   ├── index.html               ← Full IDE layout
│   ├── style.css                ← Dark theme, pipeline bar, all styling
│   └── script.js                ← Editor, tab system, API calls, rendering
│
├── examples/                    ← Sample Simply programs
│   ├── hello.simply
│   ├── variables.simply
│   ├── conditions.simply
│   └── loops.simply
│
└── package.json                 ← Node.js dependencies and scripts
```

---

## 10. How to Build and Run

### Prerequisites

- [Node.js](https://nodejs.org/) v14 or later
- [GCC](https://gcc.gnu.org/) (MinGW on Windows, or system GCC on Linux/macOS)

### Step 1 — Install Node.js dependencies

```bash
npm install
```

### Step 2 — Build the compiler (if needed)

The pre-built `simply-compiler.exe` is already included. To rebuild from source:

```bash
npm run build:compiler
```

This runs:

```bash
gcc -O2 -std=c11 -o simply-compiler.exe \
  src/main.c src/lexer.c src/parser.c src/ast.c \
  src/semantic.c src/symbol_table.c src/tac.c \
  src/optimizer.c src/codegen.c -lm
```

### Step 3 — Start the server

```bash
npm start
```

### Step 4 — Open the browser

```
http://localhost:3000
```

---

## 11. Example Programs

### Hello World

```
show "Hello, World!"
```

### Variables and Arithmetic

```
make x = 10
make y = 25
make z = x + y
make product = x * y
show z
show product
```

### Conditional

```
make age = 21
check age >= 18
    show "You are an adult."
otherwise
    show "You are a minor."
finish
```

### Loop

```
make i = 0
repeat 5
    make i = i + 1
    show i
finish
```

### Fibonacci Sequence

```
make a = 0
make b = 1
repeat 8
    make c = a + b
    make a = b
    make b = c
    show a
finish
```

### Boolean and Logic

```
make x = 5
make y = 10
make isLess = x < y
make both = x > 3 && y < 20
show isLess
show both
```

### Nested Conditions

```
make score = 85
check score >= 90
    show "Grade: A"
otherwise
    check score >= 75
        show "Grade: B"
    otherwise
        show "Grade: C"
    finish
finish
```

---

## 12. Technology Stack

| Component | Technology | Purpose |
|---|---|---|
| Compiler core | C (C11 standard) | Implements all 6 compiler phases |
| Build tool | GCC | Compiles the compiler itself and the generated C code |
| Backend server | Node.js + Express | REST API, file I/O, process management |
| Session isolation | UUID (npm package) | Each compile request gets its own temp directory |
| Frontend editor | CodeMirror 5 | Syntax-highlighted code editor |
| Frontend fonts | JetBrains Mono, Inter | Monospace editor font, UI font |
| Styling | Plain CSS (custom properties) | Dark IDE theme, pipeline visualizer |
| Frontend logic | Vanilla JavaScript | Tab system, API calls, result rendering |
