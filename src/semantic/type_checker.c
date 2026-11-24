#include "type_checker.h"

#include <stdio.h>

static const char *type_name(ASTDataType type)
{
    switch (type)
    {
    case AST_TYPE_INT:
        return "int";
    case AST_TYPE_FLOAT:
        return "float";
    case AST_TYPE_BOOL:
        return "bool";
    case AST_TYPE_CHAR:
        return "char";
    }

    return "unknown";
}

static void report_error(TypeChecker *checker, const char *message)
{
    fprintf(stderr, "SEMANTIC ERROR: %s\n", message);
    checker->error_count++;
}

static int is_numeric(ASTDataType type)
{
    return type == AST_TYPE_INT || type == AST_TYPE_FLOAT;
}

/* Check whether a value can be assigned to a target type. */
static int is_assignable(ASTDataType target, ASTDataType source)
{
    if (target == source)
    {
        return 1;
    }

    /* TYPED-MINI permits implicit int-to-float conversion. */
    return target == AST_TYPE_FLOAT && source == AST_TYPE_INT;
}

/* Forward declaration. */
static int check_statement(TypeChecker *checker, ASTNode *node);

/* Determine the type of an expression. */
static int check_expression(
    TypeChecker *checker,
    ASTNode *node,
    ASTDataType *result)
{
    if (node == NULL)
    {
        return 0;
    }

    switch (node->type)
    {

    case AST_INTEGER_LITERAL:
        *result = AST_TYPE_INT;
        return 1;

    case AST_FLOAT_LITERAL:
        *result = AST_TYPE_FLOAT;
        return 1;

    case AST_BOOLEAN_LITERAL:
        *result = AST_TYPE_BOOL;
        return 1;

    case AST_CHARACTER_LITERAL:
        *result = AST_TYPE_CHAR;
        return 1;

    case AST_IDENTIFIER:
    {
        Symbol *symbol = symbol_table_lookup(
            &checker->symbol_table,
            node->data.identifier_name);

        if (symbol == NULL)
        {
            fprintf(
                stderr,
                "SEMANTIC ERROR: Variable '%s' is not declared.\n",
                node->data.identifier_name);

            checker->error_count++;
            return 0;
        }

        *result = symbol->data_type;
        return 1;
    }

    case AST_BINARY_EXPRESSION:
    {
        ASTDataType left_type;
        ASTDataType right_type;

        if (!check_expression(
                checker,
                node->data.binary_expression.left,
                &left_type))
        {
            return 0;
        }

        if (!check_expression(
                checker,
                node->data.binary_expression.right,
                &right_type))
        {
            return 0;
        }

        ASTOperator op =
            node->data.binary_expression.operator;

        switch (op)
        {

        case AST_OP_ADD:
        case AST_OP_SUBTRACT:
        case AST_OP_MULTIPLY:
        case AST_OP_DIVIDE:

            if (!is_numeric(left_type) ||
                !is_numeric(right_type))
            {
                report_error(
                    checker,
                    "Arithmetic operators require numeric operands.");
                return 0;
            }

            *result =
                (left_type == AST_TYPE_FLOAT ||
                 right_type == AST_TYPE_FLOAT)
                    ? AST_TYPE_FLOAT
                    : AST_TYPE_INT;

            return 1;

        case AST_OP_LESS:
        case AST_OP_GREATER:
        case AST_OP_LESS_EQUAL:
        case AST_OP_GREATER_EQUAL:

            if (!is_numeric(left_type) ||
                !is_numeric(right_type))
            {
                report_error(
                    checker,
                    "Relational operators require numeric operands.");
                return 0;
            }

            *result = AST_TYPE_BOOL;
            return 1;

        case AST_OP_EQUAL:
        case AST_OP_NOT_EQUAL:

            if (left_type != right_type &&
                !(is_numeric(left_type) &&
                  is_numeric(right_type)))
            {
                report_error(
                    checker,
                    "Equality operators require compatible operands.");
                return 0;
            }

            *result = AST_TYPE_BOOL;
            return 1;

        case AST_OP_AND:
        case AST_OP_OR:

            if (left_type != AST_TYPE_BOOL ||
                right_type != AST_TYPE_BOOL)
            {
                report_error(
                    checker,
                    "Logical operators require boolean operands.");
                return 0;
            }

            *result = AST_TYPE_BOOL;
            return 1;

        default:
            report_error(
                checker,
                "Unsupported binary operator.");
            return 0;
        }
    }

    case AST_UNARY_EXPRESSION:
    {
        ASTDataType operand_type;

        if (!check_expression(
                checker,
                node->data.unary_expression.operand,
                &operand_type))
        {
            return 0;
        }

        ASTOperator op =
            node->data.unary_expression.operator;

        if (op == AST_OP_NOT)
        {
            if (operand_type != AST_TYPE_BOOL)
            {
                report_error(
                    checker,
                    "Logical NOT requires a boolean operand.");
                return 0;
            }

            *result = AST_TYPE_BOOL;
            return 1;
        }

        if (op == AST_OP_NEGATE)
        {
            if (!is_numeric(operand_type))
            {
                report_error(
                    checker,
                    "Unary minus requires a numeric operand.");
                return 0;
            }

            *result = operand_type;
            return 1;
        }

        report_error(checker, "Unsupported unary operator.");
        return 0;
    }

    default:
        report_error(
            checker,
            "Invalid expression node.");
        return 0;
    }
}

/* Check declarations, assignments and control-flow statements. */
static int check_statement(TypeChecker *checker, ASTNode *node)
{
    if (node == NULL)
    {
        return 1;
    }

    switch (node->type)
    {

    case AST_PROGRAM:
    case AST_BLOCK:
    {
        int is_block = node->type == AST_BLOCK;

        if (is_block)
        {
            symbol_table_enter_scope(&checker->symbol_table);
        }

        ASTNodeList *current =
            node->data.sequence.statements;

        while (current != NULL)
        {
            check_statement(checker, current->node);
            current = current->next;
        }

        if (is_block)
        {
            symbol_table_exit_scope(&checker->symbol_table);
        }

        return checker->error_count == 0;
    }

    case AST_VARIABLE_DECLARATION:
    {
        ASTDataType declared_type =
            node->data.variable_declaration.data_type;

        const char *name =
            node->data.variable_declaration.name;

        if (!symbol_table_declare(
                &checker->symbol_table,
                name,
                declared_type))
        {
            fprintf(
                stderr,
                "SEMANTIC ERROR: Variable '%s' is already declared in this scope.\n",
                name);

            checker->error_count++;
            return 0;
        }

        ASTNode *initializer =
            node->data.variable_declaration.initializer;

        if (initializer != NULL)
        {
            ASTDataType initializer_type;

            if (!check_expression(
                    checker,
                    initializer,
                    &initializer_type))
            {
                return 0;
            }

            if (!is_assignable(declared_type, initializer_type))
            {
                fprintf(
                    stderr,
                    "SEMANTIC ERROR: Cannot initialize '%s' of type %s with type %s.\n",
                    name,
                    type_name(declared_type),
                    type_name(initializer_type));

                checker->error_count++;
                return 0;
            }
        }

        return 1;
    }

    case AST_ASSIGNMENT:
    {
        const char *name = node->data.assignment.name;

        Symbol *symbol = symbol_table_lookup(
            &checker->symbol_table,
            name);

        if (symbol == NULL)
        {
            fprintf(
                stderr,
                "SEMANTIC ERROR: Variable '%s' is not declared.\n",
                name);

            checker->error_count++;
            return 0;
        }

        ASTDataType value_type;

        if (!check_expression(
                checker,
                node->data.assignment.value,
                &value_type))
        {
            return 0;
        }

        if (!is_assignable(symbol->data_type, value_type))
        {
            fprintf(
                stderr,
                "SEMANTIC ERROR: Cannot assign %s to variable '%s' of type %s.\n",
                type_name(value_type),
                name,
                type_name(symbol->data_type));

            checker->error_count++;
            return 0;
        }

        return 1;
    }

    case AST_IF_STATEMENT:
    {
        ASTDataType condition_type;

        if (!check_expression(
                checker,
                node->data.if_statement.condition,
                &condition_type))
        {
            return 0;
        }

        if (condition_type != AST_TYPE_BOOL)
        {
            report_error(
                checker,
                "An if condition must have type bool.");
        }

        check_statement(
            checker,
            node->data.if_statement.then_branch);

        if (node->data.if_statement.else_branch != NULL)
        {
            check_statement(
                checker,
                node->data.if_statement.else_branch);
        }

        return checker->error_count == 0;
    }

    case AST_WHILE_STATEMENT:
    {
        ASTDataType condition_type;

        if (!check_expression(
                checker,
                node->data.while_statement.condition,
                &condition_type))
        {
            return 0;
        }

        if (condition_type != AST_TYPE_BOOL)
        {
            report_error(
                checker,
                "A while condition must have type bool.");
        }

        check_statement(
            checker,
            node->data.while_statement.body);

        return checker->error_count == 0;
    }

    default:
        report_error(
            checker,
            "Unsupported statement node.");

        return 0;
    }
}

void type_checker_init(TypeChecker *checker)
{
    checker->error_count = 0;
    symbol_table_init(&checker->symbol_table);
}

void type_checker_destroy(TypeChecker *checker)
{
    symbol_table_destroy(&checker->symbol_table);
}

int type_check_program(TypeChecker *checker, ASTNode *program)
{
    if (checker == NULL || program == NULL)
    {
        return 0;
    }

    check_statement(checker, program);

    if (checker->error_count == 0)
    {
        printf("Semantic analysis successful.\n");
        return 1;
    }

    fprintf(
        stderr,
        "Semantic analysis failed with %d error(s).\n",
        checker->error_count);

    return 0;
}