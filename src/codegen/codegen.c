#include "codegen.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* =========================================
   MEMORY LAYOUT
   ========================================= */

typedef struct MemorySlot
{
    char *name;
    long offset;

    struct MemorySlot *next;

} MemorySlot;

typedef struct
{
    MemorySlot *slots;
    long frame_size;

} ActivationFrame;

typedef struct
{
    FILE *output;
    ActivationFrame frame;
    int failed;

} CodeGenerator;

/* =========================================
   STRING UTILITIES
   ========================================= */

static char *duplicate_string(const char *text)
{
    if (text == NULL)
    {
        return NULL;
    }

    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, text, length);

    return copy;
}

/* =========================================
   OPERAND CLASSIFICATION
   ========================================= */

static int is_integer_literal(const char *text)
{
    if (text == NULL || *text == '\0')
    {
        return 0;
    }

    if (*text == '-')
    {
        text++;
    }

    if (*text == '\0')
    {
        return 0;
    }

    while (*text)
    {
        if (!isdigit((unsigned char)*text))
        {
            return 0;
        }

        text++;
    }

    return 1;
}

static int is_float_literal(const char *text)
{
    if (text == NULL || *text == '\0')
    {
        return 0;
    }

    char *end = NULL;

    errno = 0;

    strtod(text, &end);

    return errno == 0 &&
           end != text &&
           *end == '\0' &&
           strchr(text, '.') != NULL;
}

static int is_boolean_literal(const char *text)
{
    return text != NULL &&
           (strcmp(text, "true") == 0 ||
            strcmp(text, "false") == 0);
}

static int is_character_literal(const char *text)
{
    return text != NULL &&
           strlen(text) == 3 &&
           text[0] == '\'' &&
           text[2] == '\'';
}

/* =========================================
   ACTIVATION FRAME MANAGEMENT
   ========================================= */

static void frame_init(ActivationFrame *frame)
{
    frame->slots = NULL;
    frame->frame_size = 0;
}

static MemorySlot *frame_find_slot(
    ActivationFrame *frame,
    const char *name)
{
    MemorySlot *current = frame->slots;

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

static int frame_allocate_slot(
    ActivationFrame *frame,
    const char *name)
{
    if (name == NULL || *name == '\0')
    {
        return 0;
    }

    if (frame_find_slot(frame, name) != NULL)
    {
        return 1;
    }

    MemorySlot *slot = malloc(sizeof(MemorySlot));

    if (slot == NULL)
    {
        return 0;
    }

    slot->name = duplicate_string(name);

    if (slot->name == NULL)
    {
        free(slot);
        return 0;
    }

    frame->frame_size += CODEGEN_SLOT_SIZE;

    slot->offset = frame->frame_size;

    slot->next = frame->slots;

    frame->slots = slot;

    return 1;
}

static void frame_destroy(ActivationFrame *frame)
{
    MemorySlot *current = frame->slots;

    while (current != NULL)
    {
        MemorySlot *next = current->next;

        free(current->name);
        free(current);

        current = next;
    }

    frame->slots = NULL;
    frame->frame_size = 0;
}

/* =========================================
   REGISTER / MEMORY OPERANDS
   ========================================= */

static void print_memory_operand(
    FILE *output,
    const char *name,
    ActivationFrame *frame)
{
    MemorySlot *slot = frame_find_slot(frame, name);

    if (slot == NULL)
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Unknown memory location '%s'.\n",
            name);

        return;
    }

    fprintf(
        output,
        "[FP-%ld]",
        slot->offset);
}

static void load_operand(
    CodeGenerator *generator,
    const char *operand,
    const char *reg)
{
    if (operand == NULL)
    {
        generator->failed = 1;
        return;
    }

    FILE *output = generator->output;

    if (is_integer_literal(operand))
    {
        fprintf(
            output,
            "    LOADI %s, %s\n",
            reg,
            operand);
    }
    else if (is_float_literal(operand))
    {
        fprintf(
            output,
            "    LOADF %s, %s\n",
            reg,
            operand);
    }
    else if (is_boolean_literal(operand))
    {
        fprintf(
            output,
            "    LOADI %s, %d\n",
            reg,
            strcmp(operand, "true") == 0 ? 1 : 0);
    }
    else if (is_character_literal(operand))
    {
        fprintf(
            output,
            "    LOADI %s, %d\n",
            reg,
            (unsigned char)operand[1]);
    }
    else
    {
        fprintf(output, "    LOAD %s, ", reg);

        print_memory_operand(
            output,
            operand,
            &generator->frame);

        fprintf(output, "\n");
    }
}

static void store_result(
    CodeGenerator *generator,
    const char *name,
    const char *reg)
{
    if (name == NULL)
    {
        generator->failed = 1;
        return;
    }

    fprintf(
        generator->output,
        "    STORE ");

    print_memory_operand(
        generator->output,
        name,
        &generator->frame);

    fprintf(
        generator->output,
        ", %s\n",
        reg);
}

/* =========================================
   FRAME CONSTRUCTION
   ========================================= */

static int build_activation_frame(
    ActivationFrame *frame,
    const TACProgram *program)
{
    TACInstruction *current = program->head;

    while (current != NULL)
    {
        if (current->result != NULL &&
            current->operation != TAC_LABEL &&
            current->operation != TAC_GOTO &&
            current->operation != TAC_IF_FALSE_GOTO)
        {
            if (!frame_allocate_slot(
                    frame,
                    current->result))
            {
                return 0;
            }
        }

        current = current->next;
    }

    return 1;
}

/* =========================================
   TARGET INSTRUCTION GENERATION
   ========================================= */

static void generate_assignment(
    CodeGenerator *generator,
    TACInstruction *instruction)
{
    load_operand(
        generator,
        instruction->operand1,
        "R0");

    store_result(
        generator,
        instruction->result,
        "R0");
}

static void generate_binary_operation(
    CodeGenerator *generator,
    TACInstruction *instruction,
    const char *target_operation)
{
    load_operand(
        generator,
        instruction->operand1,
        "R0");

    load_operand(
        generator,
        instruction->operand2,
        "R1");

    fprintf(
        generator->output,
        "    %s R2, R0, R1\n",
        target_operation);

    store_result(
        generator,
        instruction->result,
        "R2");
}

static void generate_unary_operation(
    CodeGenerator *generator,
    TACInstruction *instruction,
    const char *target_operation)
{
    load_operand(
        generator,
        instruction->operand1,
        "R0");

    fprintf(
        generator->output,
        "    %s R2, R0\n",
        target_operation);

    store_result(
        generator,
        instruction->result,
        "R2");
}

/* =========================================
   TAC TO TARGET OPERATION MAPPING
   ========================================= */

static void generate_instruction(
    CodeGenerator *generator,
    TACInstruction *instruction)
{
    if (instruction == NULL || generator->failed)
    {
        return;
    }

    FILE *output = generator->output;

    switch (instruction->operation)
    {
    case TAC_ASSIGN:

        generate_assignment(
            generator,
            instruction);

        break;

    case TAC_ADD:

        generate_binary_operation(
            generator,
            instruction,
            instruction->result_type == AST_TYPE_FLOAT
                ? "FADD"
                : "ADD");

        break;

    case TAC_SUBTRACT:

        generate_binary_operation(
            generator,
            instruction,
            instruction->result_type == AST_TYPE_FLOAT
                ? "FSUB"
                : "SUB");

        break;

    case TAC_MULTIPLY:

        generate_binary_operation(
            generator,
            instruction,
            instruction->result_type == AST_TYPE_FLOAT
                ? "FMUL"
                : "MUL");

        break;

    case TAC_DIVIDE:

        generate_binary_operation(
            generator,
            instruction,
            instruction->result_type == AST_TYPE_FLOAT
                ? "FDIV"
                : "DIV");

        break;

    case TAC_LESS:

        generate_binary_operation(
            generator,
            instruction,
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_LT"
                : "CMP_LT");

        break;

    case TAC_GREATER:

        generate_binary_operation(
            generator,
            instruction,
            "CMP_GT");

        break;

    case TAC_LESS_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            "CMP_LE");

        break;

    case TAC_GREATER_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            "CMP_GE");

        break;

    case TAC_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            "CMP_EQ");

        break;

    case TAC_NOT_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            "CMP_NE");

        break;

    case TAC_AND:

        generate_binary_operation(
            generator,
            instruction,
            "AND");

        break;

    case TAC_OR:

        generate_binary_operation(
            generator,
            instruction,
            "OR");

        break;

    case TAC_NEGATE:

        generate_unary_operation(
            generator,
            instruction,
            "NEG");

        break;

    case TAC_NOT:

        generate_unary_operation(
            generator,
            instruction,
            "NOT");

        break;

    case TAC_INT_TO_FLOAT:

        load_operand(
            generator,
            instruction->operand1,
            "R0");

        fprintf(
            output,
            "    ITOF R2, R0\n");

        store_result(
            generator,
            instruction->result,
            "R2");

        break;

    case TAC_LABEL:

        fprintf(
            output,
            "%s:\n",
            instruction->result);

        break;

    case TAC_GOTO:

        fprintf(
            output,
            "    JMP %s\n",
            instruction->result);

        break;

    case TAC_IF_FALSE_GOTO:

        load_operand(
            generator,
            instruction->operand1,
            "R0");

        fprintf(
            output,
            "    JZ R0, %s\n",
            instruction->result);

        break;

    case TAC_NOP:

        break;

    default:

        fprintf(
            stderr,
            "CODEGEN ERROR: Unsupported TAC operation.\n");

        generator->failed = 1;

        break;
    }
}

/* =========================================
   PUBLIC CODE GENERATION FUNCTIONS
   ========================================= */

int codegen_generate(
    const TACProgram *program,
    const char *output_path)
{
    if (program == NULL || output_path == NULL)
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Invalid input.\n");

        return 0;
    }

    FILE *output = fopen(output_path, "w");

    if (output == NULL)
    {
        perror("CODEGEN ERROR: Unable to create output file");
        return 0;
    }

    CodeGenerator generator;

    generator.output = output;
    generator.failed = 0;

    frame_init(&generator.frame);

    if (!build_activation_frame(
            &generator.frame,
            program))
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Unable to construct activation frame.\n");

        generator.failed = 1;
    }

    if (!generator.failed)
    {
        fprintf(
            output,
            "; =====================================\n");

        fprintf(
            output,
            "; TYPED-MINI TARGET ASSEMBLY\n");

        fprintf(
            output,
            "; Register-based virtual machine\n");

        fprintf(
            output,
            "; =====================================\n\n");

        fprintf(
            output,
            ".FRAME %ld bytes\n\n",
            generator.frame.frame_size);

        fprintf(
            output,
            "START:\n");

        TACInstruction *current = program->head;

        while (current != NULL && !generator.failed)
        {
            generate_instruction(
                &generator,
                current);

            current = current->next;
        }

        fprintf(
            output,
            "\n    HALT\n");
    }

    frame_destroy(&generator.frame);

    fclose(output);

    if (generator.failed)
    {
        remove(output_path);

        return 0;
    }

    printf(
        "Target code generation successful: %s\n",
        output_path);

    return 1;
}

void codegen_print(FILE *output)
{
    if (output == NULL)
    {
        return;
    }

    int character;

    while ((character = fgetc(output)) != EOF)
    {
        putchar(character);
    }
}