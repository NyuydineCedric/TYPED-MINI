#ifndef AST_H
#define AST_H

#include <stdbool.h>

/* Supported data types */
typedef enum {
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_BOOL,
    AST_TYPE_CHAR
} ASTDataType;

/* Supported operators */
typedef enum {
    AST_OP_ADD,
    AST_OP_SUBTRACT,
    AST_OP_MULTIPLY,
    AST_OP_DIVIDE,

    AST_OP_LESS,
    AST_OP_GREATER,
    AST_OP_LESS_EQUAL,
    AST_OP_GREATER_EQUAL,

    AST_OP_EQUAL,
    AST_OP_NOT_EQUAL,

    AST_OP_AND,
    AST_OP_OR,

    AST_OP_NEGATE,
    AST_OP_NOT
} ASTOperator;

/* AST node categories */
typedef enum {
    AST_PROGRAM,
    AST_BLOCK,

    AST_VARIABLE_DECLARATION,
    AST_ASSIGNMENT,

    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,

    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,

    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_BOOLEAN_LITERAL,
    AST_CHARACTER_LITERAL,

    AST_IDENTIFIER
} ASTNodeType;

/* Forward declaration */
typedef struct ASTNode ASTNode;

/* Linked list of AST nodes */
typedef struct ASTNodeList {
    ASTNode *node;
    struct ASTNodeList *next;
} ASTNodeList;

/* Main AST node structure */
struct ASTNode {

    ASTNodeType type;

    union {

        struct {
            ASTNodeList *statements;
        } sequence;

        struct {
            ASTDataType data_type;
            char *name;
            ASTNode *initializer;
        } variable_declaration;

        struct {
            char *name;
            ASTNode *value;
        } assignment;

        struct {
            ASTNode *condition;
            ASTNode *then_branch;
            ASTNode *else_branch;
        } if_statement;

        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_statement;

        struct {
            ASTOperator operator;
            ASTNode *left;
            ASTNode *right;
        } binary_expression;

        struct {
            ASTOperator operator;
            ASTNode *operand;
        } unary_expression;

        long integer_value;

        double float_value;

        bool boolean_value;

        char character_value;

        char *identifier_name;

    } data;
};

/* AST construction functions */

ASTNode *ast_new_program(ASTNodeList *statements);

ASTNode *ast_new_block(ASTNodeList *statements);

ASTNode *ast_new_variable_declaration(
    ASTDataType data_type,
    const char *name,
    ASTNode *initializer
);

ASTNode *ast_new_assignment(
    const char *name,
    ASTNode *value
);

ASTNode *ast_new_if_statement(
    ASTNode *condition,
    ASTNode *then_branch,
    ASTNode *else_branch
);

ASTNode *ast_new_while_statement(
    ASTNode *condition,
    ASTNode *body
);

ASTNode *ast_new_binary_expression(
    ASTOperator operator,
    ASTNode *left,
    ASTNode *right
);

ASTNode *ast_new_unary_expression(
    ASTOperator operator,
    ASTNode *operand
);

ASTNode *ast_new_integer_literal(long value);

ASTNode *ast_new_float_literal(double value);

ASTNode *ast_new_boolean_literal(bool value);

ASTNode *ast_new_character_literal(char value);

ASTNode *ast_new_identifier(const char *name);

/* AST list management */
ASTNodeList *ast_list_append(ASTNodeList *list, ASTNode *node);

/* Memory management */
void ast_free(ASTNode *node);

#endif