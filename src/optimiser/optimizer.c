#include "optimizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Count TAC instructions */
static unsigned int count_instructions(TACProgram *program)
{
    unsigned int count = 0;

    TACInstruction *current = program->head;

    while (current != NULL)
    {
        count++;
        current = current->next;
    }

    return count;
}

/* Check whether an instruction is already a leader */
static int is_leader(
    TACInstruction **leaders,
    unsigned int count,
    TACInstruction *instruction)
{
    for (unsigned int i = 0; i < count; i++)
    {
        if (leaders[i] == instruction)
        {
            return 1;
        }
    }

    return 0;
}

/* Add an instruction to the leader list */
static void add_leader(
    TACInstruction **leaders,
    unsigned int *count,
    TACInstruction *instruction)
{
    if (instruction == NULL ||
        is_leader(leaders, *count, instruction))
    {
        return;
    }

    leaders[*count] = instruction;
    (*count)++;
}

/* Find a basic block containing a particular label */
static BasicBlock *find_label_block(
    ControlFlowGraph *cfg,
    const char *label)
{
    BasicBlock *current = cfg->blocks;

    while (current != NULL)
    {

        TACInstruction *instruction = current->first;

        if (instruction->operation == TAC_LABEL &&
            instruction->result != NULL &&
            strcmp(instruction->result, label) == 0)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/* Add a successor without creating duplicate edges */
static void add_successor(
    BasicBlock *block,
    BasicBlock *successor)
{
    if (block == NULL || successor == NULL)
    {
        return;
    }

    for (unsigned int i = 0; i < block->successor_count; i++)
    {
        if (block->successors[i] == successor)
        {
            return;
        }
    }

    if (block->successor_count < 2)
    {
        block->successors[block->successor_count] = successor;
        block->successor_count++;
    }
}

/* Build basic blocks and construct the control-flow graph */
ControlFlowGraph *optimizer_build_cfg(TACProgram *program)
{
    if (program == NULL || program->head == NULL)
    {
        fprintf(stderr, "OPTIMIZER ERROR: Empty TAC program.\n");
        return NULL;
    }

    unsigned int instruction_count = count_instructions(program);

    TACInstruction **leaders = calloc(
        instruction_count,
        sizeof(TACInstruction *));

    if (leaders == NULL)
    {
        fprintf(stderr, "OPTIMIZER ERROR: Memory allocation failed.\n");
        return NULL;
    }

    unsigned int leader_count = 0;

    /* First instruction is always a leader */
    add_leader(
        leaders,
        &leader_count,
        program->head);

    /*
     * Identify additional leaders:
     *
     * 1. Every label instruction.
     * 2. The instruction immediately following a jump.
     */
    TACInstruction *current = program->head;

    while (current != NULL)
    {

        if (current->operation == TAC_LABEL)
        {
            add_leader(
                leaders,
                &leader_count,
                current);
        }

        if (current->operation == TAC_GOTO ||
            current->operation == TAC_IF_FALSE_GOTO)
        {

            add_leader(
                leaders,
                &leader_count,
                current->next);
        }

        current = current->next;
    }

    /* Allocate the control-flow graph */
    ControlFlowGraph *cfg = calloc(1, sizeof(ControlFlowGraph));

    if (cfg == NULL)
    {
        fprintf(stderr, "OPTIMIZER ERROR: Memory allocation failed.\n");
        free(leaders);
        return NULL;
    }

    BasicBlock *last_block = NULL;

    /* Construct the basic blocks */
    for (unsigned int i = 0; i < leader_count; i++)
    {

        BasicBlock *block = calloc(1, sizeof(BasicBlock));

        if (block == NULL)
        {
            fprintf(stderr, "OPTIMIZER ERROR: Memory allocation failed.\n");

            free(leaders);
            optimizer_free_cfg(cfg);

            return NULL;
        }

        block->id = cfg->block_count + 1;
        block->first = leaders[i];

        TACInstruction *instruction = leaders[i];

        while (instruction != NULL)
        {

            block->last = instruction;

            if (i + 1 < leader_count &&
                instruction->next == leaders[i + 1])
            {
                break;
            }

            instruction = instruction->next;
        }

        if (cfg->blocks == NULL)
        {
            cfg->blocks = block;
            cfg->entry = block;
        }
        else
        {
            last_block->next = block;
        }

        last_block = block;
        cfg->block_count++;
    }

    free(leaders);

    /* Connect basic blocks */
    BasicBlock *block = cfg->blocks;

    while (block != NULL)
    {

        TACInstruction *last = block->last;

        BasicBlock *fallthrough = block->next;

        if (last->operation == TAC_GOTO)
        {

            /* Unconditional jump */
            BasicBlock *target = find_label_block(
                cfg,
                last->result);

            add_successor(block, target);
        }
        else if (last->operation == TAC_IF_FALSE_GOTO)
        {

            /* Conditional jump */
            BasicBlock *target = find_label_block(
                cfg,
                last->result);

            add_successor(block, target);

            /* Execution can also continue to the next block */
            add_successor(block, fallthrough);
        }
        else
        {

            /* Normal execution continues to the next block */
            add_successor(block, fallthrough);
        }

        block = block->next;
    }

    return cfg;
}

/* Display the control-flow graph */
void optimizer_print_cfg(const ControlFlowGraph *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    printf("\n========== CONTROL-FLOW GRAPH ==========\n");
    printf("Total basic blocks: %u\n", cfg->block_count);

    BasicBlock *block = cfg->blocks;

    while (block != NULL)
    {
        printf("\nB%u:\n", block->id);

        TACInstruction *instruction = block->first;

        while (instruction != NULL)
        {
            switch (instruction->operation)
            {
            case TAC_LABEL:
                printf("    %s:\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "");
                break;

            case TAC_GOTO:
                printf("    GOTO %s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "");
                break;

            case TAC_IF_FALSE_GOTO:
                printf("    IF_FALSE %s GOTO %s\n",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "",
                       instruction->result != NULL
                           ? instruction->result
                           : "");
                break;

            case TAC_ASSIGN:
                printf("    %s = %s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "");
                break;

            case TAC_ADD:
            case TAC_SUBTRACT:
            case TAC_MULTIPLY:
            case TAC_DIVIDE:
            case TAC_LESS:
            case TAC_GREATER:
            case TAC_LESS_EQUAL:
            case TAC_GREATER_EQUAL:
            case TAC_EQUAL:
            case TAC_NOT_EQUAL:
            case TAC_AND:
            case TAC_OR:
            {
                const char *operator_symbol = "?";

                switch (instruction->operation)
                {
                case TAC_ADD:
                    operator_symbol = "+";
                    break;
                case TAC_SUBTRACT:
                    operator_symbol = "-";
                    break;
                case TAC_MULTIPLY:
                    operator_symbol = "*";
                    break;
                case TAC_DIVIDE:
                    operator_symbol = "/";
                    break;
                case TAC_LESS:
                    operator_symbol = "<";
                    break;
                case TAC_GREATER:
                    operator_symbol = ">";
                    break;
                case TAC_LESS_EQUAL:
                    operator_symbol = "<=";
                    break;
                case TAC_GREATER_EQUAL:
                    operator_symbol = ">=";
                    break;
                case TAC_EQUAL:
                    operator_symbol = "==";
                    break;
                case TAC_NOT_EQUAL:
                    operator_symbol = "!=";
                    break;
                case TAC_AND:
                    operator_symbol = "&&";
                    break;
                case TAC_OR:
                    operator_symbol = "||";
                    break;
                default:
                    break;
                }

                printf("    %s = %s %s %s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "",
                       operator_symbol,
                       instruction->operand2 != NULL
                           ? instruction->operand2
                           : "");
                break;
            }

            case TAC_NEGATE:
                printf("    %s = -%s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "");
                break;

            case TAC_NOT:
                printf("    %s = !%s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "");
                break;

            case TAC_INT_TO_FLOAT:
                printf("    %s = INT_TO_FLOAT %s\n",
                       instruction->result != NULL
                           ? instruction->result
                           : "",
                       instruction->operand1 != NULL
                           ? instruction->operand1
                           : "");
                break;

            case TAC_NOP:
                break;
            }

            if (instruction == block->last)
            {
                break;
            }

            instruction = instruction->next;
        }

        printf("    Successors: ");

        if (block->successor_count == 0)
        {
            printf("None");
        }

        for (unsigned int i = 0;
             i < block->successor_count;
             i++)
        {
            printf("B%u ", block->successors[i]->id);
        }

        printf("\n");

        block = block->next;
    }

    printf("\n========================================\n");
}

/* Release CFG structures without freeing the original TAC */
void optimizer_free_cfg(ControlFlowGraph *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    BasicBlock *current = cfg->blocks;

    while (current != NULL)
    {

        BasicBlock *next = current->next;

        free(current);

        current = next;
    }

    free(cfg);
}

/* ============================================================
   OPTIMISATION SUPPORT FUNCTIONS
   ============================================================ */

#include <errno.h>
#include <limits.h>

/* Stores a value associated with a variable or temporary. */
typedef struct ValueEntry
{
    char *name;
    char *value;
    struct ValueEntry *next;
} ValueEntry;

/* Duplicate a string safely. */
static char *optimizer_copy_string(const char *text)
{
    if (text == NULL)
        return NULL;

    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy != NULL)
        memcpy(copy, text, length);

    return copy;
}

/* Replace an instruction operand safely. */
static int replace_operand(char **target, const char *replacement)
{
    char *copy = optimizer_copy_string(replacement);

    if (copy == NULL)
        return 0;

    free(*target);
    *target = copy;

    return 1;
}

/* Find a stored value. */
static const char *find_value(ValueEntry *values, const char *name)
{
    while (values != NULL)
    {
        if (strcmp(values->name, name) == 0)
            return values->value;

        values = values->next;
    }

    return NULL;
}

/* Remove an entry by name. */
static void remove_value(ValueEntry **values, const char *name)
{
    ValueEntry **current = values;

    while (*current != NULL)
    {
        if (strcmp((*current)->name, name) == 0)
        {
            ValueEntry *removed = *current;

            *current = removed->next;

            free(removed->name);
            free(removed->value);
            free(removed);

            return;
        }

        current = &(*current)->next;
    }
}

/* Store or update a value. */
static int set_value(
    ValueEntry **values,
    const char *name,
    const char *value)
{
    remove_value(values, name);

    ValueEntry *entry = malloc(sizeof(ValueEntry));

    if (entry == NULL)
        return 0;

    entry->name = optimizer_copy_string(name);
    entry->value = optimizer_copy_string(value);

    if (entry->name == NULL || entry->value == NULL)
    {
        free(entry->name);
        free(entry->value);
        free(entry);

        return 0;
    }

    entry->next = *values;
    *values = entry;

    return 1;
}

/* Release stored values. */
static void free_values(ValueEntry *values)
{
    while (values != NULL)
    {
        ValueEntry *next = values->next;

        free(values->name);
        free(values->value);
        free(values);

        values = next;
    }
}

/* Check whether a string represents an integer. */
static int parse_integer(const char *text, long long *value)
{
    if (text == NULL || *text == '\0')
        return 0;

    errno = 0;

    char *end;

    long long result = strtoll(text, &end, 10);

    if (errno == ERANGE || *end != '\0')
        return 0;

    *value = result;

    return 1;
}

/* Check whether a string represents a numeric value. */
static int parse_number(const char *text, double *value)
{
    if (text == NULL || *text == '\0')
        return 0;

    errno = 0;

    char *end;

    double result = strtod(text, &end);

    if (errno == ERANGE || *end != '\0')
        return 0;

    *value = result;

    return 1;
}

/* ============================================================
   CONSTANT FOLDING
   ============================================================ */

/*
 * Evaluate arithmetic expressions whose operands are constants.
 *
 * Example:
 *     t1 = 20 * 2
 *
 * Becomes:
 *     t1 = 40
 */

static int fold_expression(
    TACOperation operation,
    const char *left,
    const char *right,
    char *result,
    size_t result_size)
{
    long long left_integer;
    long long right_integer;

    /* Preserve integer arithmetic when both operands are integers. */
    if (parse_integer(left, &left_integer) &&
        parse_integer(right, &right_integer))
    {
        long long folded;

        switch (operation)
        {
        case TAC_ADD:
            if (__builtin_add_overflow(
                    left_integer, right_integer, &folded))
                return 0;
            break;

        case TAC_SUBTRACT:
            if (__builtin_sub_overflow(
                    left_integer, right_integer, &folded))
                return 0;
            break;

        case TAC_MULTIPLY:
            if (__builtin_mul_overflow(
                    left_integer, right_integer, &folded))
                return 0;
            break;

        case TAC_DIVIDE:
            if (right_integer == 0 ||
                (left_integer == LLONG_MIN && right_integer == -1))
                return 0;

            folded = left_integer / right_integer;
            break;

        default:
            return 0;
        }

        snprintf(result, result_size, "%lld", folded);

        return 1;
    }

    /* Handle floating-point arithmetic. */
    double a;
    double b;

    if (!parse_number(left, &a) || !parse_number(right, &b))
        return 0;

    double folded;

    switch (operation)
    {
    case TAC_ADD:
        folded = a + b;
        break;

    case TAC_SUBTRACT:
        folded = a - b;
        break;

    case TAC_MULTIPLY:
        folded = a * b;
        break;

    case TAC_DIVIDE:
        if (b == 0.0)
            return 0;

        folded = a / b;
        break;

    default:
        return 0;
    }

    if (snprintf(result, result_size, "%.15g", folded) < 0)
        return 0;

    return 1;
}

/* Perform constant folding inside one basic block. */
static void fold_basic_block(BasicBlock *block)
{
    ValueEntry *constants = NULL;

    TACInstruction *instruction = block->first;

    while (instruction != NULL)
    {
        /*
         * Replace operands with previously known constants.
         */
        if (instruction->operand1 != NULL)
        {
            const char *value = find_value(
                constants,
                instruction->operand1);

            if (value != NULL)
                replace_operand(&instruction->operand1, value);
        }

        if (instruction->operand2 != NULL)
        {
            const char *value = find_value(
                constants,
                instruction->operand2);

            if (value != NULL)
                replace_operand(&instruction->operand2, value);
        }

        /*
         * A new assignment invalidates the previous value
         * associated with its destination.
         */
        if (instruction->result != NULL)
            remove_value(&constants, instruction->result);

        /*
         * Attempt to fold arithmetic expressions.
         */
        if (instruction->operation == TAC_ADD ||
            instruction->operation == TAC_SUBTRACT ||
            instruction->operation == TAC_MULTIPLY ||
            instruction->operation == TAC_DIVIDE)
        {
            char folded[128];

            if (fold_expression(
                    instruction->operation,
                    instruction->operand1,
                    instruction->operand2,
                    folded,
                    sizeof(folded)))
            {
                instruction->operation = TAC_ASSIGN;

                free(instruction->operand2);
                instruction->operand2 = NULL;

                replace_operand(&instruction->operand1, folded);
            }
        }

        /*
         * Remember constant assignments.
         */
        if (instruction->operation == TAC_ASSIGN &&
            instruction->result != NULL &&
            instruction->operand1 != NULL)
        {
            double ignored;

            if (parse_number(instruction->operand1, &ignored))
            {
                set_value(
                    &constants,
                    instruction->result,
                    instruction->operand1);
            }
        }

        if (instruction == block->last)
            break;

        instruction = instruction->next;
    }

    free_values(constants);
}

/*
 * Run constant folding over every basic block.
 */
void optimizer_constant_folding(ControlFlowGraph *cfg)
{
    if (cfg == NULL)
        return;

    BasicBlock *block = cfg->blocks;

    while (block != NULL)
    {
        fold_basic_block(block);
        block = block->next;
    }

    printf("Constant folding completed.\n");
}

/* ============================================================
   COPY PROPAGATION
   ============================================================ */

/*
 * Determine whether a name represents a temporary.
 * Our TAC generator uses names such as t1, t2, t3.
 */
static int is_temporary(const char *name)
{
    if (name == NULL || name[0] != 't')
        return 0;

    if (name[1] == '\0')
        return 0;

    for (unsigned int i = 1; name[i] != '\0'; i++)
    {
        if (name[i] < '0' || name[i] > '9')
            return 0;
    }

    return 1;
}

/* Replace an operand using known copies. */
static void propagate_operand(
    char **operand,
    ValueEntry *copies)
{
    if (operand == NULL || *operand == NULL)
        return;

    /*
     * Follow copy chains.
     */
    for (unsigned int i = 0; i < 100; i++)
    {
        const char *replacement = find_value(copies, *operand);

        if (replacement == NULL ||
            strcmp(replacement, *operand) == 0)
            break;

        replace_operand(operand, replacement);
    }
}

/* Perform copy propagation inside one basic block. */
static void propagate_basic_block(BasicBlock *block)
{
    ValueEntry *copies = NULL;

    TACInstruction *instruction = block->first;

    while (instruction != NULL)
    {
        /*
         * Replace operands using known copies.
         */
        propagate_operand(&instruction->operand1, copies);
        propagate_operand(&instruction->operand2, copies);

        /*
         * If a variable is modified, invalidate copies
         * that depend on its previous value.
         */
        if (instruction->result != NULL)
        {
            remove_value(&copies, instruction->result);

            ValueEntry **current = &copies;

            while (*current != NULL)
            {
                if (strcmp(
                        (*current)->value,
                        instruction->result) == 0)
                {
                    ValueEntry *removed = *current;

                    *current = removed->next;

                    free(removed->name);
                    free(removed->value);
                    free(removed);
                }
                else
                {
                    current = &(*current)->next;
                }
            }
        }

        /*
         * Record temporary copies.
         *
         * Example:
         *     t1 = age
         *
         * Future uses of t1 can use age directly,
         * provided age has not changed.
         */
        if (instruction->operation == TAC_ASSIGN &&
            instruction->result != NULL &&
            instruction->operand1 != NULL &&
            is_temporary(instruction->result) &&
            strcmp(
                instruction->result,
                instruction->operand1) != 0)
        {
            if (set_value(
                    &copies,
                    instruction->result,
                    instruction->operand1))
            {
                /*
                 * Eliminate the redundant copy.
                 */
                instruction->operation = TAC_NOP;

                free(instruction->result);
                free(instruction->operand1);
                free(instruction->operand2);

                instruction->result = NULL;
                instruction->operand1 = NULL;
                instruction->operand2 = NULL;
            }
        }

        if (instruction == block->last)
            break;

        instruction = instruction->next;
    }

    free_values(copies);
}

/*
 * Run copy propagation over every basic block.
 */
void optimizer_copy_propagation(ControlFlowGraph *cfg)
{
    if (cfg == NULL)
        return;

    BasicBlock *block = cfg->blocks;

    while (block != NULL)
    {
        propagate_basic_block(block);
        block = block->next;
    }

    printf("Copy propagation completed.\n");
}

/* ============================================================
   COMPLETE OPTIMISATION PIPELINE
   ============================================================ */

void optimizer_run(ControlFlowGraph *cfg)
{
    if (cfg == NULL)
        return;

    printf("\nStarting optimisation...\n");

    optimizer_constant_folding(cfg);

    optimizer_copy_propagation(cfg);

    printf("Optimisation completed.\n");
}