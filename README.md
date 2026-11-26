# TYPED-MINI Compiler

A mini-compiler implemented in C17 for the CENP4111 – Introduction to Compiler Construction course at the University of Bamenda.

TYPED-MINI demonstrates the major phases of compiler construction, from lexical and syntax analysis to semantic checking, intermediate representation, optimisation and target code generation.

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Features](#2-features)
3. [Supported Language](#3-supported-language)
4. [Compiler Architecture](#4-compiler-architecture)
5. [Project Structure](#5-project-structure)
6. [Requirements](#6-requirements)
7. [Installation and Build](#7-installation-and-build)
8. [Running the Compiler](#8-running-the-compiler)
9. [Example Programs](#9-example-programs)
10. [Testing](#10-testing)
11. [Intermediate Representation](#11-intermediate-representation)
12. [Optimisation](#12-optimisation)
13. [Target Code Generation](#13-target-code-generation)
14. [Error Handling](#14-error-handling)
15. [Current Limitations](#15-current-limitations)
16. [Technologies Used](#16-technologies-used)

---

## 1. Project Overview

TYPED-MINI is a compiler for a small, statically typed programming language.

The project demonstrates how a compiler transforms source code through several processing stages:

- Lexical analysis
- Syntax analysis
- Abstract Syntax Tree (AST) construction
- Semantic analysis
- Three-Address Code (TAC) generation
- Control-Flow Graph (CFG) construction
- Code optimisation
- Target code generation

The compiler is implemented in C17 and uses Flex for lexical analysis and GNU Bison for syntax analysis.

Its purpose is to demonstrate fundamental compiler construction concepts through a working implementation.

## 2. Features

### Frontend

- Tokenisation using Flex.
- Syntax analysis using GNU Bison.
- Abstract Syntax Tree construction.
- Support for primitive data types.
- Variable declarations and assignments.
- Arithmetic and logical expressions.
- Conditional statements.
- While loops.
- Semantic and type checking.

### Intermediate Representation

- Three-Address Code generation.
- Temporary variable generation.
- Control-Flow Graph construction.

### Optimisation

- Constant folding.
- Copy propagation.
- Sethi–Ullman-based expression evaluation optimisation.

### Backend

- Target assembly generation.
- Register-based virtual machine instruction format.
- Register management and protection.
- Stack-frame memory slot allocation.

### Testing

- Valid source program tests.
- Invalid source program tests.
- Automated test execution using a shell script.

## 3. Supported Language

TYPED-MINI supports the following language constructs.

### 3.1 Data Types

| Type    | Description           |
| ------- | --------------------- |
| `int`   | Integer values        |
| `float` | Floating-point values |
| `bool`  | Boolean values        |
| `char`  | Character values      |

### 3.2 Variable Declarations

Variables can be declared with or without initial values.

```c
int age;
int score = 20;
float height = 1.75;
bool active = true;
char grade = 'A';
```

### 3.3 Assignments

```c
int number = 10;

number = 25;
```

### 3.4 Arithmetic Operators

| Operator | Operation      |
| -------- | -------------- |
| `+`      | Addition       |
| `-`      | Subtraction    |
| `*`      | Multiplication |
| `/`      | Division       |

Example:

```c
int a = 10;
int b = 5;
int result = a + b * 2;
```

### 3.5 Relational Operators

| Operator | Operation             |
| -------- | --------------------- |
| `<`      | Less than             |
| `>`      | Greater than          |
| `<=`     | Less than or equal    |
| `>=`     | Greater than or equal |

### 3.6 Equality Operators

| Operator | Operation    |
| -------- | ------------ |
| `==`     | Equal to     |
| `!=`     | Not equal to |

### 3.7 Logical Operators

| Operator | Operation   |
| -------- | ----------- |
| `&&`     | Logical AND |
| `\|\|`   | Logical OR  |
| `!`      | Logical NOT |

### 3.8 Unary Operators

- Unary minus: `-`
- Unary plus: `+`
- Logical NOT: `!`

### 3.9 Conditional Statements

The language supports `if` and `if-else` statements.

```c
int age = 20;

if (age >= 18) {
    age = age + 1;
} else {
    age = 0;
}
```

### 3.10 While Loops

```c
int count = 0;

while (count < 5) {
    count = count + 1;
}
```

### 3.11 Blocks

Multiple statements can be grouped using braces.

```c
{
    int x = 10;
    int y = 20;
    int result = x + y;
}
```

### 3.12 Comments

Single-line comments are supported.

```c
// This is a comment
int value = 10;
```

---

## 4. Compiler Architecture

TYPED-MINI follows a multi-phase compiler architecture.

```text
              SOURCE PROGRAM
                     |
                     v
             LEXICAL ANALYSIS
                  (Flex)
                     |
                     v
              SYNTAX ANALYSIS
                 (Bison)
                     |
                     v
          ABSTRACT SYNTAX TREE
                   (AST)
                     |
                     v
             SEMANTIC ANALYSIS
              (Type Checker)
                     |
                     v
          THREE-ADDRESS CODE
                   (TAC)
                     |
                     v
          CONTROL-FLOW GRAPH
                   (CFG)
                     |
                     v
                OPTIMISATION
                     |
                     v
            TARGET CODE GENERATION
                     |
                     v
                OUTPUT.TASM
```

### Phase 1: Lexical Analysis

The lexical analyser reads the source program and identifies tokens such as:

- Keywords
- Identifiers
- Literals
- Operators
- Delimiters

Flex is used to implement the scanner.

### Phase 2: Syntax Analysis

The parser checks whether the sequence of tokens follows the language grammar.

GNU Bison is used to generate the parser.

During parsing, the compiler constructs an Abstract Syntax Tree.

### Phase 3: Abstract Syntax Tree

The AST represents the syntactic structure of the source program.

It provides a structured representation that subsequent compiler phases can process.

### Phase 4: Semantic Analysis

The semantic analyser checks the meaning and type correctness of the program.

It handles checks including:

- Variable declarations.
- Duplicate declarations.
- Undeclared variable references.
- Type compatibility.
- Validity of operations involving different data types.

### Phase 5: Three-Address Code Generation

The compiler translates the AST into an intermediate representation called Three-Address Code.

TAC breaks complex expressions into simpler instructions involving operands and temporary variables.

### Phase 6: Control-Flow Graph Construction

The generated intermediate representation is used to construct a Control-Flow Graph.

The CFG represents the flow of execution between program blocks and provides a structure for optimisation.

### Phase 7: Optimisation

The compiler performs optimisation using the constructed control-flow representation.

The optimisation stage includes constant folding and copy propagation.

### Phase 8: Target Code Generation

The backend generates target assembly instructions and writes them to:

```text
output.tasm
```

---

## 5. Project Structure

```text
TYPED-MINI/
│
├── src/
│   │
│   ├── ast/
│   │   ├── ast.c
│   │   └── ast.h
│   │
│   ├── semantic/
│   │   ├── symbol_table.c
│   │   ├── symbol_table.h
│   │   ├── type_checker.c
│   │   └── type_checker.h
│   │
│   ├── ir/
│   │   ├── three_address_code.c
│   │   └── three_address_code.h
│   │
│   ├── optimiser/
│   │   ├── optimizer.c
│   │   └── optimizer.h
│   │
│   ├── codegen/
│   │   ├── codegen.c
│   │   └── codegen.h
│   │
│   ├── parser/
│   │   ├── parser.y
│   │   ├── parser.tab.c
│   │   └── parser.tab.h
│   │
│   ├── lexer/
│   │   ├── lexer.l
│   │   └── lex.yy.c
│   │
│   └── driver/
│       └── main.c
│
├── tests/
│   ├── valid/
│   └── invalid/
│
├── Makefile
├── run_tests.sh
├── README.md
└── output.tasm
```

Generated files and other repository contents may vary depending on the committed project state.

---

## 6. Requirements

The following tools are required to build and run the compiler:

| Tool      | Purpose                           |
| --------- | --------------------------------- |
| GCC       | C compiler                        |
| GNU Make  | Build automation                  |
| Flex      | Lexical analyser generator        |
| GNU Bison | Parser generator                  |
| Bash      | Running the automated test script |

The project uses the C17 language standard.

### Windows Environment

The project can be built using the MSYS2 UCRT64 environment.

Ensure GCC, Make, Flex and Bison are available in your terminal.

---

## 7. Installation and Build

### Step 1: Clone the Repository

```bash
git clone <repository-url>
```

Navigate into the project directory:

```bash
cd TYPED-MINI
```

### Step 2: Build the Compiler

Run:

```bash
make
```

The Makefile compiles the project using:

```text
-std=c17 -Wall -Wextra -Wpedantic -g
```

The generated executable is:

```text
typed-mini.exe
```

### Step 3: Clean the Build

To remove the compiled executable:

```bash
make clean
```

To rebuild:

```bash
make clean
make
```

---

## 8. Running the Compiler

The compiler accepts an optional source file as a command-line argument.

### Compile a Source File

```bash
./typed-mini.exe tests/valid/tac_arithmetic.tmini
```

The compiler performs its analysis and code-generation phases.

On successful compilation, it reports:

```text
COMPILATION SUCCESSFUL!
```

The generated target assembly is saved to:

```text
output.tasm
```

### Run Without a Source File

The compiler can also read source code from standard input:

```bash
./typed-mini.exe
```

Enter the source program through standard input.

---

## 9. Example Programs

### Example 1: Arithmetic Operations

```c
int a = 10;
int b = 20;
int result = a + b;
```

### Example 2: Type Conversion

```c
int age = 20;
float height = age;
float result = height + 10.5;
```

### Example 3: Conditional Statements

```c
int score = 75;

if (score >= 50) {
    score = score + 10;
} else {
    score = 0;
}
```

### Example 4: While Loop

```c
int counter = 0;

while (counter < 10) {
    counter = counter + 1;
}
```

These examples illustrate supported language constructs. Semantic validity is determined by the compiler's type checker.

---

## 10. Testing

TYPED-MINI includes separate directories for valid and invalid programs.

```text
tests/
│
├── valid/
│   ├── copy_propagation.tmini
│   ├── float_operations.tmini
│   ├── int_to_float.tmini
│   ├── optimization_branches.tmini
│   ├── optimization_invalidation.tmini
│   ├── tac_arithmetic.tmini
│   ├── tac_control_flow.tmini
│   └── tac_type_conversion.tmini
│
└── invalid/
    ├── duplicate_declaration.tmini
    ├── float_to_int.tmini
    ├── invalid_arithmetic.tmini
    ├── invalid_condition.tmini
    ├── missing_semicolon.tmini
    ├── type_mismatch.tmini
    └── undeclared_variable.tmini
```

### Run the Complete Test Suite

First, build the compiler:

```bash
make
```

Then run:

```bash
make test
```

Alternatively:

```bash
bash run_tests.sh
```

The test script:

1. Compiles every program in `tests/valid/`.
2. Checks that every program in `tests/invalid/` is rejected.
3. Reports passed and failed tests.
4. Returns a failure status if any test fails.

The current test suite contains eight valid programs and seven invalid programs.

---

## 11. Intermediate Representation

TYPED-MINI uses Three-Address Code as its intermediate representation.

Three-Address Code represents complex expressions as sequences of simpler instructions.

For example, an expression such as:

```c
int result = a + b * c;
```

can be represented conceptually as:

```text
t1 = b * c
t2 = a + t1
result = t2
```

Using TAC provides a structured representation between source-level constructs and target code generation.

It also makes optimisation easier because individual operations can be analysed and transformed.

---

## 12. Optimisation

The compiler includes an optimisation stage based on its Control-Flow Graph representation.

Implemented optimisation techniques include:

### Constant Folding

Constant expressions are evaluated during compilation rather than being left for execution.

For example:

```text
t1 = 10 * 2
```

can be simplified to:

```text
t1 = 20
```

### Copy Propagation

Copy propagation replaces references to copied variables with their available values where appropriate.

### Sethi–Ullman-Based Expression Evaluation

Sethi–Ullman-based optimisation is used to improve expression evaluation order and reduce register requirements.

These techniques demonstrate important compiler optimisation concepts.

---

## 13. Target Code Generation

After the intermediate representation and optimisation stages, the compiler generates target assembly instructions.

The generated output is written to:

```text
output.tasm
```

The target instruction format uses a register-based virtual machine model.

The generated assembly includes operations for:

- Loading values into registers.
- Storing values in memory slots.
- Arithmetic operations.
- Type conversion.
- Register-based computation.
- Program termination.

The backend uses a fixed register set and stack-frame memory slots to represent variables and temporary values.

**Note:** The current project generates target assembly. It does not include a virtual machine or interpreter for executing the generated `.tasm` instructions.

---

## 14. Error Handling

TYPED-MINI reports errors encountered during compilation.

### Lexical Errors

The lexer reports invalid characters and their line numbers.

Example format:

```text
LEXICAL ERROR at line 3: Invalid character '@'
```

### Syntax Errors

The parser reports syntax errors using detailed Bison error messages.

### Semantic Errors

The semantic analyser rejects programs containing invalid declarations, undeclared variables and incompatible types.

Compilation stops when parsing or semantic analysis fails.

---

## 15. Current Limitations

TYPED-MINI is an educational mini-compiler and does not implement every feature found in a production programming language.

Current limitations include:

- No function declarations or function calls.
- No arrays or structures.
- No user-defined data types.
- No input/output statements.
- No complete runtime or virtual machine for executing generated target assembly.
- No complete production-level error recovery mechanism.

The language and compiler are intentionally limited to demonstrating the fundamental phases and techniques of compiler construction.

---

## 16. Technologies Used

| Technology             | Application                        |
| ---------------------- | ---------------------------------- |
| C17                    | Compiler implementation            |
| Flex                   | Lexical analysis                   |
| GNU Bison              | Syntax analysis                    |
| GCC                    | Compilation                        |
| GNU Make               | Build automation                   |
| Bash                   | Automated testing                  |
| Three-Address Code     | Intermediate representation        |
| Control-Flow Graph     | Program-flow representation        |
| Sethi–Ullman Algorithm | Expression evaluation optimisation |

---

## Academic Information

**Course:** CENP4111 – Introduction to Compiler Construction  
**Institution:** The University of Bamenda  
**College:** College of Technology (COLTECH)  
**Programme:** Bachelor of Engineering, Software Engineering  
**Project:** TYPED-MINI Compiler

---

## Conclusion

TYPED-MINI demonstrates the implementation of a multi-phase compiler using C17, Flex and GNU Bison.

The project brings together lexical analysis, syntax analysis, semantic checking, intermediate code generation, optimisation and target code generation in one integrated compiler.

It provides a practical demonstration of the fundamental principles of compiler construction.
