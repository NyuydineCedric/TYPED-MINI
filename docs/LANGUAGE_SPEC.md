# TYPED-MINI Language Specification

## 1. Introduction

TYPED-MINI is a statically typed educational programming language
designed for the implementation of a mini-compiler.

The compiler is implemented using C, Flex and Bison.

Its primary objective is to demonstrate lexical analysis,
syntax analysis, semantic analysis, intermediate code generation,
optimization and target code generation.

## 2. Supported Data Types

TYPED-MINI supports four primitive data types:

- int: Integer values.
- float: Floating-point values.
- bool: Boolean values.
- char: Single character values.

Examples:

int age = 20;
float price = 15.5;
bool active = true;
char grade = 'A';

## 3. Variables and Declarations

Variables must be declared before they are used.

Declaration syntax:

type identifier;
type identifier = expression;

Examples:

int age;
int score = 100;
float temperature = 36.5;

Variable names must begin with a letter or underscore.
Subsequent characters may include letters, digits and underscores.

Duplicate declarations within the same scope are not allowed.

## 4. Assignment

Variables can be assigned using the assignment operator (=).

Examples:

age = 25;
temperature = 37.5;

An assignment must respect the declared type of its variable.

## 5. Type System and Coercion

TYPED-MINI uses static type checking.

The following implicit conversion is permitted:

int -> float

Examples:

float result = 10;
float result = 5 + 2.5;

The following implicit conversion is prohibited:

float -> int

Incompatible assignments between bool, char and numeric
types must be rejected.

The semantic analyser is responsible for enforcing these rules.

## 6. Operators

### Arithmetic Operators

- Addition

* Subtraction

- Multiplication
  / Division

Arithmetic operations support int and float operands.

If either numeric operand is float, the operation produces
a float result.

Otherwise, the result is int.

### Relational Operators

< Less than

> Greater than
> <= Less than or equal
> = Greater than or equal

Relational operations require numeric operands and produce bool.

### Equality Operators

== Equal to
!= Not equal to

Equality comparisons support compatible operands.
Numeric operands may be compared using numeric promotion.

### Logical Operators

&& Logical AND
|| Logical OR
! Logical NOT

Logical operators require bool operands and produce bool.

## 7. Control Flow

### If Statement

if (condition) {
statements
}

### If-Else Statement

if (condition) {
statements
} else {
statements
}

Conditions must evaluate to bool.

### While Loop

while (condition) {
statements
}

The condition must evaluate to bool.

## 8. Blocks and Scope

Curly braces define blocks.

Nested blocks are permitted.

Variables declared inside a block belong to that block's scope
and are not accessible outside it.

The symbol table must support scope management.

## 9. Comments

Single-line comments begin with //.

Example:

// This is a comment

Comments are ignored during lexical analysis.

## 10. Operator Precedence

From highest to lowest:

1. Parentheses
2. Unary operators: !, -, +
3. Multiplication and division: \*, /
4. Addition and subtraction: +, -
5. Relational operators: <, >, <=, >=
6. Equality operators: ==, !=
7. Logical AND: &&
8. Logical OR: ||

Operators at the same precedence level are evaluated
left to right, except unary operators.

## 11. Language Limitations

The initial version does not support:

- Functions
- Arrays
- Strings
- Classes and objects
- Pointers
- User-defined data types

## 12. Compiler Requirements

The compiler must:

1. Recognize valid tokens.
2. Validate program syntax.
3. Construct an Abstract Syntax Tree.
4. Maintain a symbol table.
5. Perform static type checking.
6. Support int-to-float coercion.
7. Reject incompatible assignments.
8. Generate three-address code.
9. Perform meaning-preserving optimizations.
10. Generate target code.
