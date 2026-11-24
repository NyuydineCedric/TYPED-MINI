#include "three_address_code.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Variable information used during TAC generation */
typedef struct VariableBinding
{
    char *name;
    ASTDataType data_type;
    unsigned int scope_level;
    struct VariableBinding *next;
} VariableBinding;

/* Internal generator state */
typedef struct
{
    TACProgram *program;
    VariableBinding *variables;
    unsigned int scope_level;
    int failed;
} TACGenerator;

/* Allocate a copy of a string */
static char *copy_string(const char *source)
{
    if (source == NULL)
    {
        return NULL;
    }

    size_t length = strlen(source) + 1;
    char *copy = malloc(length);

    if (copy == NULL)
    {
        fprintf(stderr, "TAC ERROR: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    memcpy(copy, source, length);

    return copy;
}

/* Convert a number to a string */
static char *number_to_string(double value)
{
    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%.15g", value);

    return copy_string(buffer);
}

/* Generate a temporary variable */
static char *new_temporary(TACGenerator *generator)
{
    char buffer[32];

    generator->program->temporary_count++;

    snprintf(
        buffer,
        sizeof(buffer),
        "t%u",
        generator->program->temporary_count);

    return copy_string(buffer);
}

/* Generate a label */
static char *new_label(TACGenerator *generator)
{
    char buffer[32];

    generator->program->label_count++;

    snprintf(
        buffer,
        sizeof(buffer),
        "L%u",
        generator->program->label_count);

    return copy_string(buffer);
}

/* Add an instruction to the TAC program */
static void emit(
    TACGenerator *generator,
    TACOperation operation,
    const char *result,
    const char *operand1,
    const char *operand2)
{
    TACInstruction *instruction = calloc(1, sizeof(TACInstruction));

    if (instruction == NULL)
    {
        fprintf(stderr, "TAC ERROR: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    instruction->operation = operation;
    instruction->result = copy_string(result);
    instruction->operand1 = copy_string(operand1);
    instruction->operand2 = copy_string(operand2);

    if (generator->program->head == NULL)
    {
        generator->program->head = instruction;
    }
    else
    {
        generator->program->tail->next = instruction;
    }

    generator->program->tail = instruction;
}

/* Find the most recently declared variable */
static VariableBinding *find_variable(
    TACGenerator *generator,
    const char *name)
{
    VariableBinding *current = generator->variables;

    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/* Register a variable */
static void register_variable(
    TACGenerator *generator,
    const char *name,
    ASTDataType data_type)
{
    VariableBinding *binding = malloc(sizeof(VariableBinding));

    if (binding == NULL)
    {
        fprintf(stderr, "TAC ERROR: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    binding->name = copy_string(name);
    binding->data_type = data_type;
    binding->scope_level = generator->scope_level;

    binding->next = generator->variables;
    generator->variables = binding;
}

/* Enter a nested scope */
static void enter_scope(TACGenerator *generator)
{
    generator->scope_level++;
}

/* Leave a nested scope and remove its variables */
static void exit_scope(TACGenerator *generator)
{
    VariableBinding **current = &generator->variables;

    while (*current != NULL)
    {
        VariableBinding *binding = *current;

        if (binding->scope_level == generator->scope_level)
        {
            *current = binding->next;

            free(binding->name);
            free(binding);
        }
        else
        {
            current = &binding->next;
        }
    }

    if (generator->scope_level > 0)
    {
        generator->scope_level--;
    }
}

/* Convert an integer operand into a floating-point operand */
static char *convert_to_float(
    TACGenerator *generator,
    char *operand,
    ASTDataType source_type,
    ASTDataType target_type)
{
    if (source_type == target_type)
    {
        return operand;
    }

    if (source_type == AST_TYPE_INT &&
        target_type == AST_TYPE_FLOAT)
    {
        char *temporary = new_temporary(generator);

        emit(
            generator,
            TAC_INT_TO_FLOAT,
            temporary,
            operand,
            NULL);

        free(operand);

        return temporary;
    }

    fprintf(stderr, "TAC ERROR: Unsupported type conversion.\n");

    generator->failed = 1;

    free(operand);

    return NULL;
}

/*
 * Calculate the Sethi-Ullman number of an expression tree.
 *
 * A leaf requires one register.
 * A unary expression inherits its operand's requirement.
 * A binary expression:
 *   - requires one additional register if both children
 *     have equal requirements;
 *   - otherwise requires the maximum of the two.
 */
static unsigned int sethi_ullman_number(const ASTNode *node)
{
    if (node == NULL)
    {
        return 0;
    }

    switch (node->type)
    {
    case AST_BINARY_EXPRESSION:
    {
        unsigned int left_number =
            sethi_ullman_number(
                node->data.binary_expression.left);

        unsigned int right_number =
            sethi_ullman_number(
                node->data.binary_expression.right);

        if (left_number == right_number)
        {
            return left_number + 1;
        }

        return left_number > right_number
                   ? left_number
                   : right_number;
    }

    case AST_UNARY_EXPRESSION:
        return sethi_ullman_number(
            node->data.unary_expression.operand);

    default:
        return 1;
    }
}

/* Forward declarations */
static char *generate_expression(
    TACGenerator *generator,
    ASTNode *node,
    ASTDataType *result_type);

static void generate_statement(
    TACGenerator *generator,
    ASTNode *node);

/* Translate an expression into TAC */
static char *generate_expression(
    TACGenerator *generator,
    ASTNode *node,
    ASTDataType *result_type)
{
    if (node == NULL)
    {
        generator->failed = 1;
        return NULL;
    }

    switch (node->type)
    {
    case AST_INTEGER_LITERAL:
        *result_type = AST_TYPE_INT;

        return number_to_string(
            (double)node->data.integer_value);

    case AST_FLOAT_LITERAL:
        *result_type = AST_TYPE_FLOAT;

        return number_to_string(
            node->data.float_value);

    case AST_BOOLEAN_LITERAL:
        *result_type = AST_TYPE_BOOL;

        return copy_string(
            node->data.boolean_value ? "true" : "false");

    case AST_CHARACTER_LITERAL:
    {
        char buffer[8];

        *result_type = AST_TYPE_CHAR;

        snprintf(
            buffer,
            sizeof(buffer),
            "'%c'",
            node->data.character_value);

        return copy_string(buffer);
    }

    case AST_IDENTIFIER:
    {
        const char *name = node->data.identifier_name;

        VariableBinding *binding =
            find_variable(generator, name);

        if (binding == NULL)
        {
            fprintf(
                stderr,
                "TAC ERROR: Unknown variable '%s'.\n",
                name);

            generator->failed = 1;
            return NULL;
        }

        *result_type = binding->data_type;

        return copy_string(name);
    }

    case AST_BINARY_EXPRESSION:
    {
        ASTNode *left_node =
            node->data.binary_expression.left;

        ASTNode *right_node =
            node->data.binary_expression.right;

        ASTOperator operator =
            node->data.binary_expression.operator;

        ASTDataType left_type;
        ASTDataType right_type;

        char *left = NULL;
        char *right = NULL;

        /*
         * Sethi-Ullman evaluation ordering.
         *
         * Evaluate the subtree with the higher register
         * requirement first.
         *
         * Logical AND and OR are excluded to preserve
         * their left-to-right evaluation order.
         */
        unsigned int left_number =
            sethi_ullman_number(left_node);

        unsigned int right_number =
            sethi_ullman_number(right_node);

        int evaluate_right_first =
            right_number > left_number &&
            operator != AST_OP_AND &&
            operator != AST_OP_OR;

        if (evaluate_right_first)
        {
            right = generate_expression(
                generator,
                right_node,
                &right_type);

            if (right == NULL)
            {
                return NULL;
            }

            left = generate_expression(
                generator,
                left_node,
                &left_type);

            if (left == NULL)
            {
                free(right);
                return NULL;
            }
        }
        else
        {
            left = generate_expression(
                generator,
                left_node,
                &left_type);

            if (left == NULL)
            {
                return NULL;
            }

            right = generate_expression(
                generator,
                right_node,
                &right_type);

            if (right == NULL)
            {
                free(left);
                return NULL;
            }
        }

        /*
         * Preserve the original left and right operands
         * even when their evaluation order is changed.
         */
        int numeric_operation =
            operator == AST_OP_ADD ||
            operator == AST_OP_SUBTRACT ||
            operator == AST_OP_MULTIPLY ||
            operator == AST_OP_DIVIDE ||
            operator == AST_OP_LESS ||
            operator == AST_OP_GREATER ||
            operator == AST_OP_LESS_EQUAL ||
            operator == AST_OP_GREATER_EQUAL ||
            operator == AST_OP_EQUAL ||
            operator == AST_OP_NOT_EQUAL;

        /*
         * Apply the language's implicit int-to-float
         * conversion when numeric operand types differ.
         */
        if (numeric_operation &&
            left_type != right_type)
        {
            if (left_type == AST_TYPE_INT &&
                right_type == AST_TYPE_FLOAT)
            {
                left = convert_to_float(
                    generator,
                    left,
                    left_type,
                    AST_TYPE_FLOAT);

                if (left == NULL)
                {
                    free(right);
                    return NULL;
                }

                left_type = AST_TYPE_FLOAT;
            }
            else if (left_type == AST_TYPE_FLOAT &&
                     right_type == AST_TYPE_INT)
            {
                right = convert_to_float(
                    generator,
                    right,
                    right_type,
                    AST_TYPE_FLOAT);

                if (right == NULL)
                {
                    free(left);
                    return NULL;
                }

                right_type = AST_TYPE_FLOAT;
            }
        }

        char *temporary = new_temporary(generator);

        TACOperation tac_operation;

        switch (operator)
        {
        case AST_OP_ADD:
            tac_operation = TAC_ADD;
            *result_type =
                (left_type == AST_TYPE_FLOAT ||
                 right_type == AST_TYPE_FLOAT)
                    ? AST_TYPE_FLOAT
                    : AST_TYPE_INT;
            break;

        case AST_OP_SUBTRACT:
            tac_operation = TAC_SUBTRACT;
            *result_type =
                (left_type == AST_TYPE_FLOAT ||
                 right_type == AST_TYPE_FLOAT)
                    ? AST_TYPE_FLOAT
                    : AST_TYPE_INT;
            break;

        case AST_OP_MULTIPLY:
            tac_operation = TAC_MULTIPLY;
            *result_type =
                (left_type == AST_TYPE_FLOAT ||
                 right_type == AST_TYPE_FLOAT)
                    ? AST_TYPE_FLOAT
                    : AST_TYPE_INT;
            break;

        case AST_OP_DIVIDE:
            tac_operation = TAC_DIVIDE;
            *result_type =
                (left_type == AST_TYPE_FLOAT ||
                 right_type == AST_TYPE_FLOAT)
                    ? AST_TYPE_FLOAT
                    : AST_TYPE_INT;
            break;

        case AST_OP_LESS:
            tac_operation = TAC_LESS;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_GREATER:
            tac_operation = TAC_GREATER;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_LESS_EQUAL:
            tac_operation = TAC_LESS_EQUAL;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_GREATER_EQUAL:
            tac_operation = TAC_GREATER_EQUAL;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_EQUAL:
            tac_operation = TAC_EQUAL;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_NOT_EQUAL:
            tac_operation = TAC_NOT_EQUAL;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_AND:
            tac_operation = TAC_AND;
            *result_type = AST_TYPE_BOOL;
            break;

        case AST_OP_OR:
            tac_operation = TAC_OR;
            *result_type = AST_TYPE_BOOL;
            break;

        default:
            fprintf(
                stderr,
                "TAC ERROR: Unsupported binary operator.\n");

            generator->failed = 1;

            free(left);
            free(right);
            free(temporary);

            return NULL;
        }

        /*
         * The emitted instruction retains the correct
         * operand positions regardless of evaluation order.
         */
        emit(
            generator,
            tac_operation,
            temporary,
            left,
            right);

        free(left);
        free(right);

        return temporary;
    }

    case AST_UNARY_EXPRESSION:
    {
        ASTDataType operand_type;

        char *operand = generate_expression(
            generator,
            node->data.unary_expression.operand,
            &operand_type);

        if (operand == NULL)
        {
            return NULL;
        }

        ASTOperator operator =
            node->data.unary_expression.operator;

        TACOperation tac_operation;

        if (operator == AST_OP_NEGATE)
        {
            tac_operation = TAC_NEGATE;
            *result_type = operand_type;
        }
        else if (operator == AST_OP_NOT)
        {
            tac_operation = TAC_NOT;
            *result_type = AST_TYPE_BOOL;
        }
        else
        {
            fprintf(
                stderr,
                "TAC ERROR: Unsupported unary operator.\n");

            generator->failed = 1;
            free(operand);

            return NULL;
        }

        char *temporary = new_temporary(generator);

        emit(
            generator,
            tac_operation,
            temporary,
            operand,
            NULL);

        free(operand);

        return temporary;
    }

    default:
        fprintf(
            stderr,
            "TAC ERROR: Invalid expression node.\n");

        generator->failed = 1;

        return NULL;
    }
}

/* Translate statements into TAC */
static void generate_statement(
    TACGenerator *generator,
    ASTNode *node)
{
    if (node == NULL || generator->failed)
    {
        return;
    }

    switch (node->type)
    {
    case AST_PROGRAM:
    {
        ASTNodeList *current =
            node->data.sequence.statements;

        while (current != NULL && !generator->failed)
        {
            generate_statement(generator, current->node);
            current = current->next;
        }

        break;
    }

    case AST_BLOCK:
    {
        enter_scope(generator);

        ASTNodeList *current =
            node->data.sequence.statements;

        while (current != NULL && !generator->failed)
        {
            generate_statement(generator, current->node);
            current = current->next;
        }

        exit_scope(generator);

        break;
    }

    case AST_VARIABLE_DECLARATION:
    {
        const char *name =
            node->data.variable_declaration.name;

        ASTDataType declared_type =
            node->data.variable_declaration.data_type;

        register_variable(
            generator,
            name,
            declared_type);

        ASTNode *initializer =
            node->data.variable_declaration.initializer;

        if (initializer != NULL)
        {
            ASTDataType initializer_type;

            char *value = generate_expression(
                generator,
                initializer,
                &initializer_type);

            if (value == NULL)
            {
                break;
            }

            value = convert_to_float(
                generator,
                value,
                initializer_type,
                declared_type);

            if (value != NULL)
            {
                emit(
                    generator,
                    TAC_ASSIGN,
                    name,
                    value,
                    NULL);
            }

            free(value);
        }

        break;
    }

    case AST_ASSIGNMENT:
    {
        const char *name =
            node->data.assignment.name;

        VariableBinding *binding =
            find_variable(generator, name);

        if (binding == NULL)
        {
            fprintf(
                stderr,
                "TAC ERROR: Unknown variable '%s'.\n",
                name);

            generator->failed = 1;
            break;
        }

        ASTDataType value_type;

        char *value = generate_expression(
            generator,
            node->data.assignment.value,
            &value_type);

        if (value == NULL)
        {
            break;
        }

        value = convert_to_float(
            generator,
            value,
            value_type,
            binding->data_type);

        if (value != NULL)
        {
            emit(
                generator,
                TAC_ASSIGN,
                name,
                value,
                NULL);
        }

        free(value);

        break;
    }

    case AST_IF_STATEMENT:
    {
        ASTDataType condition_type;

        char *condition = generate_expression(
            generator,
            node->data.if_statement.condition,
            &condition_type);

        if (condition == NULL)
        {
            break;
        }

        char *else_label = new_label(generator);
        char *end_label = new_label(generator);

        emit(
            generator,
            TAC_IF_FALSE_GOTO,
            else_label,
            condition,
            NULL);

        free(condition);

        generate_statement(
            generator,
            node->data.if_statement.then_branch);

        if (node->data.if_statement.else_branch != NULL)
        {
            emit(
                generator,
                TAC_GOTO,
                end_label,
                NULL,
                NULL);

            emit(
                generator,
                TAC_LABEL,
                else_label,
                NULL,
                NULL);

            generate_statement(
                generator,
                node->data.if_statement.else_branch);

            emit(
                generator,
                TAC_LABEL,
                end_label,
                NULL,
                NULL);
        }
        else
        {
            emit(
                generator,
                TAC_LABEL,
                else_label,
                NULL,
                NULL);
        }

        free(else_label);
        free(end_label);

        break;
    }

    case AST_WHILE_STATEMENT:
    {
        char *start_label = new_label(generator);
        char *end_label = new_label(generator);

        emit(
            generator,
            TAC_LABEL,
            start_label,
            NULL,
            NULL);

        ASTDataType condition_type;

        char *condition = generate_expression(
            generator,
            node->data.while_statement.condition,
            &condition_type);

        if (condition == NULL)
        {
            free(start_label);
            free(end_label);
            break;
        }

        emit(
            generator,
            TAC_IF_FALSE_GOTO,
            end_label,
            condition,
            NULL);

        free(condition);

        generate_statement(
            generator,
            node->data.while_statement.body);

        emit(
            generator,
            TAC_GOTO,
            start_label,
            NULL,
            NULL);

        emit(
            generator,
            TAC_LABEL,
            end_label,
            NULL,
            NULL);

        free(start_label);
        free(end_label);

        break;
    }

    default:
        fprintf(
            stderr,
            "TAC ERROR: Unsupported statement node.\n");

        generator->failed = 1;

        break;
    }
}

/* Return the printable name of a TAC operation */
static const char *operation_name(TACOperation operation)
{
    switch (operation)
    {
    case TAC_ASSIGN:
        return "=";

    case TAC_ADD:
        return "+";
    case TAC_SUBTRACT:
        return "-";
    case TAC_MULTIPLY:
        return "*";
    case TAC_DIVIDE:
        return "/";

    case TAC_LESS:
        return "<";
    case TAC_GREATER:
        return ">";
    case TAC_LESS_EQUAL:
        return "<=";
    case TAC_GREATER_EQUAL:
        return ">=";

    case TAC_EQUAL:
        return "==";
    case TAC_NOT_EQUAL:
        return "!=";

    case TAC_AND:
        return "&&";
    case TAC_OR:
        return "||";

    case TAC_NEGATE:
        return "NEG";
    case TAC_NOT:
        return "!";

    case TAC_INT_TO_FLOAT:
        return "int_to_float";

    case TAC_LABEL:
        return "LABEL";
    case TAC_GOTO:
        return "GOTO";
    case TAC_IF_FALSE_GOTO:
        return "IF_FALSE_GOTO";

    case TAC_NOP:
        return "NOP";
    }

    return "UNKNOWN";
}

/* Generate TAC from an AST */
TACProgram *tac_generate(ASTNode *root)
{
    if (root == NULL || root->type != AST_PROGRAM)
    {
        fprintf(stderr, "TAC ERROR: Invalid AST root.\n");
        return NULL;
    }

    TACProgram *program = calloc(1, sizeof(TACProgram));

    if (program == NULL)
    {
        fprintf(stderr, "TAC ERROR: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    TACGenerator generator = {
        .program = program,
        .variables = NULL,
        .scope_level = 0,
        .failed = 0};

    generate_statement(&generator, root);

    while (generator.variables != NULL)
    {
        VariableBinding *binding = generator.variables;

        generator.variables = binding->next;

        free(binding->name);
        free(binding);
    }

    if (generator.failed)
    {
        tac_free(program);
        return NULL;
    }

    return program;
}

/* Print the generated TAC */
void tac_print(const TACProgram *program)
{
    if (program == NULL)
    {
        return;
    }

    TACInstruction *current = program->head;

    printf("\n========== THREE-ADDRESS CODE ==========\n");

    while (current != NULL)
    {
        const char *operation =
            operation_name(current->operation);

        switch (current->operation)
        {
        case TAC_LABEL:
            printf("%s:\n", current->result);
            break;

        case TAC_GOTO:
            printf("GOTO %s\n", current->result);
            break;

        case TAC_IF_FALSE_GOTO:
            printf(
                "IF_FALSE %s GOTO %s\n",
                current->operand1,
                current->result);
            break;

        case TAC_ASSIGN:
            printf(
                "%s = %s\n",
                current->result,
                current->operand1);
            break;

        case TAC_NEGATE:
        case TAC_NOT:
            printf(
                "%s = %s %s\n",
                current->result,
                operation,
                current->operand1);
            break;

        case TAC_INT_TO_FLOAT:
            printf(
                "%s = INT_TO_FLOAT %s\n",
                current->result,
                current->operand1);
            break;

        default:
            printf(
                "%s = %s %s %s\n",
                current->result,
                current->operand1,
                operation,
                current->operand2);
            break;
        }

        current = current->next;
    }

    printf("========================================\n");
}

/* Release all TAC instructions and their operands */
void tac_free(TACProgram *program)
{
    if (program == NULL)
    {
        return;
    }

    TACInstruction *current = program->head;

    while (current != NULL)
    {
        TACInstruction *next = current->next;

        free(current->result);
        free(current->operand1);
        free(current->operand2);
        free(current);

        current = next;
    }

    free(program);
}