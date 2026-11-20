%code requires {
    #include "../ast/ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>

#include "../ast/ast.h"

int yylex(void);
void yyerror(const char *message);

/* The completed AST will be accessible to the compiler driver. */
ASTNode *ast_root = NULL;
%}

/* Generate a detailed parser error message. */
%define parse.error detailed

/* Semantic value types. */
%union {
    long integer;
    double floating;
    char character;
    char *string;

    ASTNode *node;
    ASTNodeList *list;
    ASTDataType data_type;
}

/* Data types */
%token INT FLOAT BOOL CHAR

/* Control-flow keywords */
%token IF ELSE WHILE

/* Boolean values */
%token TRUE FALSE

/* Identifiers and literals */
%token <string> IDENTIFIER
%token <integer> INTEGER_LITERAL
%token <floating> FLOAT_LITERAL
%token <character> CHAR_LITERAL

/* Operators */
%token ASSIGN
%token PLUS MINUS MULTIPLY DIVIDE

%token LESS_THAN GREATER_THAN
%token LESS_EQUAL GREATER_EQUAL

%token EQUAL NOT_EQUAL

%token LOGICAL_AND LOGICAL_OR LOGICAL_NOT

/* Delimiters */
%token SEMICOLON COMMA
%token LEFT_PAREN RIGHT_PAREN
%token LEFT_BRACE RIGHT_BRACE

/* Semantic types for grammar rules */
%type <node> program
%type <node> statement
%type <node> block
%type <node> declaration
%type <node> assignment
%type <node> if_statement
%type <node> while_statement
%type <node> expression

%type <list> statement_list

%type <data_type> type

/* Operator precedence and associativity */
%precedence LOWER_THAN_ELSE
%precedence ELSE

%left LOGICAL_OR
%left LOGICAL_AND

%left EQUAL NOT_EQUAL

%left LESS_THAN GREATER_THAN LESS_EQUAL GREATER_EQUAL

%left PLUS MINUS

%left MULTIPLY DIVIDE

%precedence LOGICAL_NOT UMINUS

/* Free identifier strings discarded during error recovery. */
%destructor { free($$); } <string>

%%

/* A complete program consists of zero or more statements. */
program:
    statement_list
    {
        $$ = ast_new_program($1);
        ast_root = $$;
    }
    ;

/* Build a linked list of statements. */
statement_list:
    %empty
    {
        $$ = NULL;
    }
    | statement_list statement
    {
        $$ = ast_list_append($1, $2);
    }
    ;

/* Supported statements. */
statement:
    declaration
    {
        $$ = $1;
    }
    | assignment
    {
        $$ = $1;
    }
    | if_statement
    {
        $$ = $1;
    }
    | while_statement
    {
        $$ = $1;
    }
    | block
    {
        $$ = $1;
    }
    ;

/* Variable declarations. */
declaration:
    type IDENTIFIER SEMICOLON
    {
        $$ = ast_new_variable_declaration($1, $2, NULL);
        free($2);
    }
    | type IDENTIFIER ASSIGN expression SEMICOLON
    {
        $$ = ast_new_variable_declaration($1, $2, $4);
        free($2);
    }
    ;

/* Supported primitive types. */
type:
    INT
    {
        $$ = AST_TYPE_INT;
    }
    | FLOAT
    {
        $$ = AST_TYPE_FLOAT;
    }
    | BOOL
    {
        $$ = AST_TYPE_BOOL;
    }
    | CHAR
    {
        $$ = AST_TYPE_CHAR;
    }
    ;

/* Variable assignment. */
assignment:
    IDENTIFIER ASSIGN expression SEMICOLON
    {
        $$ = ast_new_assignment($1, $3);
        free($1);
    }
    ;

/* Conditional statements. */
if_statement:
    IF LEFT_PAREN expression RIGHT_PAREN statement %prec LOWER_THAN_ELSE
    {
        $$ = ast_new_if_statement($3, $5, NULL);
    }
    | IF LEFT_PAREN expression RIGHT_PAREN statement ELSE statement
    {
        $$ = ast_new_if_statement($3, $5, $7);
    }
    ;

/* While loops. */
while_statement:
    WHILE LEFT_PAREN expression RIGHT_PAREN statement
    {
        $$ = ast_new_while_statement($3, $5);
    }
    ;

/* A block contains a sequence of statements. */
block:
    LEFT_BRACE statement_list RIGHT_BRACE
    {
        $$ = ast_new_block($2);
    }
    ;

/* Expressions. */
expression:
    INTEGER_LITERAL
    {
        $$ = ast_new_integer_literal($1);
    }
    | FLOAT_LITERAL
    {
        $$ = ast_new_float_literal($1);
    }
    | TRUE
    {
        $$ = ast_new_boolean_literal(1);
    }
    | FALSE
    {
        $$ = ast_new_boolean_literal(0);
    }
    | CHAR_LITERAL
    {
        $$ = ast_new_character_literal($1);
    }
    | IDENTIFIER
    {
        $$ = ast_new_identifier($1);
        free($1);
    }
    | LEFT_PAREN expression RIGHT_PAREN
    {
        $$ = $2;
    }

    /* Arithmetic operators */
    | expression PLUS expression
    {
        $$ = ast_new_binary_expression(AST_OP_ADD, $1, $3);
    }
    | expression MINUS expression
    {
        $$ = ast_new_binary_expression(AST_OP_SUBTRACT, $1, $3);
    }
    | expression MULTIPLY expression
    {
        $$ = ast_new_binary_expression(AST_OP_MULTIPLY, $1, $3);
    }
    | expression DIVIDE expression
    {
        $$ = ast_new_binary_expression(AST_OP_DIVIDE, $1, $3);
    }

    /* Relational operators */
    | expression LESS_THAN expression
    {
        $$ = ast_new_binary_expression(AST_OP_LESS, $1, $3);
    }
    | expression GREATER_THAN expression
    {
        $$ = ast_new_binary_expression(AST_OP_GREATER, $1, $3);
    }
    | expression LESS_EQUAL expression
    {
        $$ = ast_new_binary_expression(AST_OP_LESS_EQUAL, $1, $3);
    }
    | expression GREATER_EQUAL expression
    {
        $$ = ast_new_binary_expression(AST_OP_GREATER_EQUAL, $1, $3);
    }

    /* Equality operators */
    | expression EQUAL expression
    {
        $$ = ast_new_binary_expression(AST_OP_EQUAL, $1, $3);
    }
    | expression NOT_EQUAL expression
    {
        $$ = ast_new_binary_expression(AST_OP_NOT_EQUAL, $1, $3);
    }

    /* Logical operators */
    | expression LOGICAL_AND expression
    {
        $$ = ast_new_binary_expression(AST_OP_AND, $1, $3);
    }
    | expression LOGICAL_OR expression
    {
        $$ = ast_new_binary_expression(AST_OP_OR, $1, $3);
    }

    /* Unary operators */
    | LOGICAL_NOT expression
    {
        $$ = ast_new_unary_expression(AST_OP_NOT, $2);
    }
    | MINUS expression %prec UMINUS
    {
        $$ = ast_new_unary_expression(AST_OP_NEGATE, $2);
    }
    | PLUS expression %prec UMINUS
    {
        $$ = $2;
    }
    ;

%%

/* Report syntax errors. */
void yyerror(const char *message)
{
    fprintf(stderr, "PARSER ERROR: %s\n", message);
}