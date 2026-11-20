#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocate memory for a new AST node */
static ASTNode *ast_allocate(ASTNodeType type)
{
    ASTNode *node = calloc(1, sizeof(ASTNode));

    if (node == NULL) {
        perror("AST allocation failed");
        exit(EXIT_FAILURE);
    }

    node->type = type;

    return node;
}

/* Duplicate strings safely */
static char *ast_duplicate_string(const char *text)
{
    if (text == NULL) {
        return NULL;
    }

    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy == NULL) {
        perror("AST string allocation failed");
        exit(EXIT_FAILURE);
    }

    memcpy(copy, text, length);

    return copy;
}

/* Create a program node */
ASTNode *ast_new_program(ASTNodeList *statements)
{
    ASTNode *node = ast_allocate(AST_PROGRAM);

    node->data.sequence.statements = statements;

    return node;
}

/* Create a block node */
ASTNode *ast_new_block(ASTNodeList *statements)
{
    ASTNode *node = ast_allocate(AST_BLOCK);

    node->data.sequence.statements = statements;

    return node;
}

/* Create a variable declaration */
ASTNode *ast_new_variable_declaration(
    ASTDataType data_type,
    const char *name,
    ASTNode *initializer)
{
    ASTNode *node = ast_allocate(AST_VARIABLE_DECLARATION);

    node->data.variable_declaration.data_type = data_type;

    node->data.variable_declaration.name =
        ast_duplicate_string(name);

    node->data.variable_declaration.initializer = initializer;

    return node;
}

/* Create an assignment */
ASTNode *ast_new_assignment(
    const char *name,
    ASTNode *value)
{
    ASTNode *node = ast_allocate(AST_ASSIGNMENT);

    node->data.assignment.name = ast_duplicate_string(name);

    node->data.assignment.value = value;

    return node;
}

/* Create an if statement */
ASTNode *ast_new_if_statement(
    ASTNode *condition,
    ASTNode *then_branch,
    ASTNode *else_branch)
{
    ASTNode *node = ast_allocate(AST_IF_STATEMENT);

    node->data.if_statement.condition = condition;

    node->data.if_statement.then_branch = then_branch;

    node->data.if_statement.else_branch = else_branch;

    return node;
}

/* Create a while statement */
ASTNode *ast_new_while_statement(
    ASTNode *condition,
    ASTNode *body)
{
    ASTNode *node = ast_allocate(AST_WHILE_STATEMENT);

    node->data.while_statement.condition = condition;

    node->data.while_statement.body = body;

    return node;
}

/* Create a binary expression */
ASTNode *ast_new_binary_expression(
    ASTOperator operator,
    ASTNode *left,
    ASTNode *right)
{
    ASTNode *node = ast_allocate(AST_BINARY_EXPRESSION);

    node->data.binary_expression.operator = operator;

    node->data.binary_expression.left = left;

    node->data.binary_expression.right = right;

    return node;
}

/* Create a unary expression */
ASTNode *ast_new_unary_expression(
    ASTOperator operator,
    ASTNode *operand)
{
    ASTNode *node = ast_allocate(AST_UNARY_EXPRESSION);

    node->data.unary_expression.operator = operator;

    node->data.unary_expression.operand = operand;

    return node;
}

/* Create an integer literal */
ASTNode *ast_new_integer_literal(long value)
{
    ASTNode *node = ast_allocate(AST_INTEGER_LITERAL);

    node->data.integer_value = value;

    return node;
}

/* Create a floating-point literal */
ASTNode *ast_new_float_literal(double value)
{
    ASTNode *node = ast_allocate(AST_FLOAT_LITERAL);

    node->data.float_value = value;

    return node;
}

/* Create a Boolean literal */
ASTNode *ast_new_boolean_literal(bool value)
{
    ASTNode *node = ast_allocate(AST_BOOLEAN_LITERAL);

    node->data.boolean_value = value;

    return node;
}

/* Create a character literal */
ASTNode *ast_new_character_literal(char value)
{
    ASTNode *node = ast_allocate(AST_CHARACTER_LITERAL);

    node->data.character_value = value;

    return node;
}

/* Create an identifier */
ASTNode *ast_new_identifier(const char *name)
{
    ASTNode *node = ast_allocate(AST_IDENTIFIER);

    node->data.identifier_name = ast_duplicate_string(name);

    return node;
}

/* Append a node to an AST linked list */
ASTNodeList *ast_list_append(ASTNodeList *list, ASTNode *node)
{
    ASTNodeList *new_item = malloc(sizeof(ASTNodeList));

    if (new_item == NULL) {
        perror("AST list allocation failed");
        exit(EXIT_FAILURE);
    }

    new_item->node = node;
    new_item->next = NULL;

    if (list == NULL) {
        return new_item;
    }

    ASTNodeList *current = list;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = new_item;

    return list;
}

/* Free an entire AST list */
static void ast_free_list(ASTNodeList *list)
{
    while (list != NULL) {

        ASTNodeList *next = list->next;

        ast_free(list->node);

        free(list);

        list = next;
    }
}

/* Free an AST recursively */
void ast_free(ASTNode *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {

        case AST_PROGRAM:
        case AST_BLOCK:
            ast_free_list(node->data.sequence.statements);
            break;

        case AST_VARIABLE_DECLARATION:
            free(node->data.variable_declaration.name);

            ast_free(node->data.variable_declaration.initializer);
            break;

        case AST_ASSIGNMENT:
            free(node->data.assignment.name);

            ast_free(node->data.assignment.value);
            break;

        case AST_IF_STATEMENT:
            ast_free(node->data.if_statement.condition);

            ast_free(node->data.if_statement.then_branch);

            ast_free(node->data.if_statement.else_branch);
            break;

        case AST_WHILE_STATEMENT:
            ast_free(node->data.while_statement.condition);

            ast_free(node->data.while_statement.body);
            break;

        case AST_BINARY_EXPRESSION:
            ast_free(node->data.binary_expression.left);

            ast_free(node->data.binary_expression.right);
            break;

        case AST_UNARY_EXPRESSION:
            ast_free(node->data.unary_expression.operand);
            break;

        case AST_IDENTIFIER:
            free(node->data.identifier_name);
            break;

        case AST_INTEGER_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_BOOLEAN_LITERAL:
        case AST_CHARACTER_LITERAL:
            break;
    }

    free(node);
}