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

/*
 * Register descriptor.
 *
 * Tracks the value currently held in a physical register.
 *
 * valid:
 *     1 if the register contains a tracked value.
 *     0 if the register is available.
 *
 * value_name:
 *     Name of the variable or temporary in the register.
 *
 * last_used:
 *     Used by the allocator to select the least
 *     recently used register when all are occupied.
 */
typedef struct
{
    char *value_name;
    unsigned long last_used;
    int valid;
    int protected;

} RegisterDescriptor;

/*
 * Code generator state.
 */
typedef struct
{
    FILE *output;

    ActivationFrame frame;

    /*
     * Physical register descriptors.
     */
    RegisterDescriptor registers[CODEGEN_REGISTER_COUNT];

    /*
     * Logical clock for tracking register usage.
     */
    unsigned long register_clock;

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
   REGISTER ALLOCATION
   ========================================= */

/*
 * Physical register names supported by our
 * virtual target machine.
 */
static const char *REGISTER_NAMES[CODEGEN_REGISTER_COUNT] = {
    "R0",
    "R1",
    "R2"};

/*
 * Release a register descriptor.
 *
 * The actual register contents do not need to
 * be erased because values are preserved in
 * activation-frame memory.
 */
static void register_invalidate(
    RegisterDescriptor *reg)
{
    if (reg == NULL)
    {
        return;
    }

    free(reg->value_name);

    reg->value_name = NULL;
    reg->last_used = 0;
    reg->valid = 0;
    reg->protected = 0;
}

/*
 * Clear all register descriptors.
 *
 * Used at control-flow boundaries where
 * cached register information cannot safely
 * be assumed to remain valid.
 */
static void register_clear_all(
    CodeGenerator *generator)
{
    if (generator == NULL)
    {
        return;
    }

    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        register_invalidate(&generator->registers[i]);
    }
}

/*
 * Protect a register from being selected
 * for reuse during the current instruction.
 */
static void register_protect(
    CodeGenerator *generator,
    int register_index)
{
    if (generator == NULL ||
        register_index < 0 ||
        register_index >= CODEGEN_REGISTER_COUNT)
    {
        return;
    }

    generator->registers[register_index].protected = 1;
}

/*
 * Allow the allocator to reuse a register again.
 */
static void register_unprotect(
    CodeGenerator *generator,
    int register_index)
{
    if (generator == NULL ||
        register_index < 0 ||
        register_index >= CODEGEN_REGISTER_COUNT)
    {
        return;
    }

    generator->registers[register_index].protected = 0;
}

/*
 * Find a register containing a particular value.
 *
 * Returns the register index, or -1 if the
 * value is not currently cached.
 */
static int register_find_value(
    CodeGenerator *generator,
    const char *value_name)
{
    if (generator == NULL || value_name == NULL)
    {
        return -1;
    }

    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        RegisterDescriptor *reg = &generator->registers[i];

        if (reg->valid &&
            reg->value_name != NULL &&
            strcmp(reg->value_name, value_name) == 0)
        {
            reg->last_used = ++generator->register_clock;

            return i;
        }
    }

    return -1;
}

/*
 * Acquire a physical register.
 *
 * Allocation policy:
 *
 * 1. Prefer an invalid register.
 * 2. If all registers are occupied, select
 *    the least recently used register.
 *
 * This is safe because our generated instructions
 * write results through to activation-frame memory.
 */
/*
 * Acquire a physical register.
 *
 * Allocation policy:
 *
 * 1. Never select a protected register.
 * 2. Prefer an available register.
 * 3. Otherwise select the least recently used
 *    unprotected register.
 *
 * Returns -1 if no register is available.
 */
static int register_acquire(
    CodeGenerator *generator)
{
    if (generator == NULL)
    {
        return -1;
    }

    /*
     * First look for an available register.
     */
    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        RegisterDescriptor *reg = &generator->registers[i];

        if (!reg->valid && !reg->protected)
        {
            return i;
        }
    }

    /*
     * Find the least recently used unprotected
     * register.
     */
    int selected = -1;

    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        RegisterDescriptor *reg = &generator->registers[i];

        if (reg->protected)
        {
            continue;
        }

        if (selected == -1 ||
            reg->last_used <
                generator->registers[selected].last_used)
        {
            selected = i;
        }
    }

    if (selected == -1)
    {
        return -1;
    }

    register_invalidate(&generator->registers[selected]);

    return selected;
}

/*
 * Associate a value with a register.
 *
 * A value must have a memory slot before it
 * can safely be cached.
 */
static int register_bind(
    CodeGenerator *generator,
    int register_index,
    const char *value_name)
{
    if (generator == NULL ||
        register_index < 0 ||
        register_index >= CODEGEN_REGISTER_COUNT ||
        value_name == NULL)
    {
        return 0;
    }

    RegisterDescriptor *reg =
        &generator->registers[register_index];

    char *copy = duplicate_string(value_name);

    if (copy == NULL)
    {
        generator->failed = 1;
        return 0;
    }

    free(reg->value_name);

    reg->value_name = copy;
    reg->valid = 1;
    reg->protected = 0;
    reg->last_used = ++generator->register_clock;

    return 1;
}

/*
 * Invalidate every cached copy of a value.
 *
 * This is important when an assignment changes
 * the value of an existing variable.
 */
static void register_invalidate_value(
    CodeGenerator *generator,
    const char *value_name)
{
    if (generator == NULL || value_name == NULL)
    {
        return;
    }

    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        RegisterDescriptor *reg = &generator->registers[i];

        if (reg->valid &&
            reg->value_name != NULL &&
            strcmp(reg->value_name, value_name) == 0)
        {
            register_invalidate(reg);
        }
    }
}

/*
 * Get the textual name of a physical register.
 */
static const char *register_name(int register_index)
{
    if (register_index < 0 ||
        register_index >= CODEGEN_REGISTER_COUNT)
    {
        return NULL;
    }

    return REGISTER_NAMES[register_index];
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

    if (errno != 0 || end == text || *end != '\0')
    {
        return 0;
    }

    /*
     * A floating-point literal may contain:
     *
     *  12.5
     *  12.0
     *  1.5e10
     *  2e-5
     *
     * Scientific notation does not necessarily
     * contain a decimal point.
     */

    return strchr(text, '.') != NULL ||
           strchr(text, 'e') != NULL ||
           strchr(text, 'E') != NULL;
}

static int is_numeric_literal(const char *text)
{
    return is_integer_literal(text) ||
           is_float_literal(text);
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
    if (frame == NULL || name == NULL)
    {
        return NULL;
    }

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
    if (frame == NULL ||
        name == NULL ||
        *name == '\0')
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
    if (frame == NULL)
    {
        return;
    }

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

/*
 * Print a memory operand.
 *
 * Returns:
 *  1 if successful.
 *  0 if the memory location does not exist.
 */

static int print_memory_operand(
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
            name != NULL ? name : "(null)");

        return 0;
    }

    fprintf(
        output,
        "[FP-%ld]",
        slot->offset);

    return 1;
}

/*
 * Load an operand into a register.
 *
 * The operand type is important because a floating-point
 * value such as 20 may look like an integer literal.
 *
 * AST_TYPE_FLOAT ensures that LOADF is used in that case.
 */

/*
 * Load an operand and return the register index
 * containing its value.
 *
 * If the operand is already cached, reuse its register.
 * Otherwise, allocate a register and load it.
 *
 * Returns -1 on failure.
 */
static int load_operand(
    CodeGenerator *generator,
    const char *operand,
    ASTDataType operand_type)
{
    if (generator == NULL || operand == NULL)
    {
        if (generator != NULL)
            generator->failed = 1;

        return -1;
    }

    /*
     * Check whether this variable or temporary
     * is already available in a register.
     */
    if (!is_numeric_literal(operand) &&
        !is_boolean_literal(operand) &&
        !is_character_literal(operand))
    {
        int cached = register_find_value(generator, operand);

        if (cached >= 0)
        {
            return cached;
        }
    }

    /*
     * Allocate a register for the operand.
     */
    int register_index = register_acquire(generator);

    if (register_index < 0)
    {
        generator->failed = 1;
        return -1;
    }

    const char *reg = register_name(register_index);

    FILE *output = generator->output;

    if (operand_type == AST_TYPE_FLOAT &&
        is_numeric_literal(operand))
    {
        fprintf(output, "    LOADF %s, %s\n", reg, operand);
    }
    else if (is_integer_literal(operand))
    {
        fprintf(output, "    LOADI %s, %s\n", reg, operand);
    }
    else if (is_float_literal(operand))
    {
        fprintf(output, "    LOADF %s, %s\n", reg, operand);
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

        if (!print_memory_operand(
                output,
                operand,
                &generator->frame))
        {
            generator->failed = 1;
            return -1;
        }

        fprintf(output, "\n");

        if (!register_bind(
                generator,
                register_index,
                operand))
        {
            return -1;
        }
    }

    /*
     * Reserve registers containing literals too.
     *
     * Otherwise, the allocator may incorrectly
     * reuse a register while its literal value
     * is still needed by the current instruction.
     */
    if (is_numeric_literal(operand) ||
        is_boolean_literal(operand) ||
        is_character_literal(operand))
    {
        if (!register_bind(
                generator,
                register_index,
                operand))
        {
            return -1;
        }
    }

    return register_index;
}

/*
 * Store a register value in a memory slot.
 */

/*
 * Store a register value in its activation-frame
 * memory slot and cache the result in that register.
 */
static void store_result(
    CodeGenerator *generator,
    const char *name,
    int register_index)
{
    if (generator == NULL ||
        name == NULL ||
        register_index < 0 ||
        register_index >= CODEGEN_REGISTER_COUNT)
    {
        if (generator != NULL)
            generator->failed = 1;

        return;
    }

    const char *reg = register_name(register_index);

    /*
     * Ensure the destination has a memory slot.
     */
    if (!frame_allocate_slot(&generator->frame, name))
    {
        generator->failed = 1;
        return;
    }

    /*
     * Write through to memory.
     *
     * This ensures that an evicted register can
     * always be safely reloaded later.
     */
    fprintf(generator->output, "    STORE ");

    if (!print_memory_operand(
            generator->output,
            name,
            &generator->frame))
    {
        generator->failed = 1;
        return;
    }

    fprintf(
        generator->output,
        ", %s\n",
        reg);

    /*
     * Remove any previous cached copy of the
     * destination before associating it with
     * the new register.
     */
    register_invalidate_value(generator, name);

    register_bind(
        generator,
        register_index,
        name);
}

/* =========================================
   FRAME CONSTRUCTION
   ========================================= */

static int build_activation_frame(
    ActivationFrame *frame,
    const TACProgram *program)
{
    if (frame == NULL || program == NULL)
    {
        return 0;
    }

    TACInstruction *current = program->head;

    while (current != NULL)
    {
        /*
         * Labels and jumps do not need memory slots.
         */

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

/*
 * Generate assignment:
 *
 *     result = operand1
 */

static void generate_assignment(
    CodeGenerator *generator,
    TACInstruction *instruction)
{
    int source_register = load_operand(
        generator,
        instruction->operand1,
        instruction->operand1_type);

    if (source_register < 0)
    {
        return;
    }

    store_result(
        generator,
        instruction->result,
        source_register);
}

/*
 * Generate binary operations:
 *
 *     result = operand1 OP operand2
 *
 * Registers:
 *     R0 = first operand
 *     R1 = second operand
 *     R2 = result
 */

static void generate_binary_operation(
    CodeGenerator *generator,
    TACInstruction *instruction,
    const char *target_operation)
{
    /*
     * Load the first operand.
     */
    int left_register = load_operand(
        generator,
        instruction->operand1,
        instruction->operand1_type);

    if (left_register < 0)
    {
        return;
    }

    /*
     * Protect the first operand before loading
     * the second operand.
     */
    register_protect(generator, left_register);

    /*
     * Load the second operand.
     */
    int right_register = load_operand(
        generator,
        instruction->operand2,
        instruction->operand2_type);

    if (right_register < 0)
    {
        register_unprotect(generator, left_register);
        return;
    }

    /*
     * Protect the second operand.
     */
    register_protect(generator, right_register);

    /*
     * Allocate a register for the result.
     *
     * Protected operand registers cannot be
     * selected by the allocator.
     */
    int result_register = register_acquire(generator);

    if (result_register < 0)
    {
        generator->failed = 1;

        register_unprotect(generator, left_register);
        register_unprotect(generator, right_register);

        return;
    }

    /*
     * Generate the target instruction.
     */
    fprintf(
        generator->output,
        "    %s %s, %s, %s\n",
        target_operation,
        register_name(result_register),
        register_name(left_register),
        register_name(right_register));

    /*
     * Store and cache the result.
     */
    store_result(
        generator,
        instruction->result,
        result_register);

    /*
     * Release protected operand registers.
     */
    register_unprotect(generator, left_register);
    register_unprotect(generator, right_register);
}

/*
 * Generate unary operations:
 *
 *     result = OP operand1
 */

static void generate_unary_operation(
    CodeGenerator *generator,
    TACInstruction *instruction,
    const char *target_operation)
{
    /*
     * Load the operand.
     */
    int operand_register = load_operand(
        generator,
        instruction->operand1,
        instruction->operand1_type);

    if (operand_register < 0)
    {
        return;
    }

    /*
     * Protect the operand.
     */
    register_protect(generator, operand_register);

    /*
     * Allocate a result register.
     */
    int result_register = register_acquire(generator);

    if (result_register < 0)
    {
        generator->failed = 1;

        register_unprotect(generator, operand_register);

        return;
    }

    /*
     * Generate the unary instruction.
     */
    fprintf(
        generator->output,
        "    %s %s, %s\n",
        target_operation,
        register_name(result_register),
        register_name(operand_register));

    /*
     * Store the result.
     */
    store_result(
        generator,
        instruction->result,
        result_register);

    /*
     * Release the protected operand.
     */
    register_unprotect(generator, operand_register);
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
        /* -------------------------------------
           ASSIGNMENT
           ------------------------------------- */

    case TAC_ASSIGN:

        generate_assignment(
            generator,
            instruction);

        break;

        /* -------------------------------------
           ARITHMETIC
           ------------------------------------- */

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

        /* -------------------------------------
           RELATIONAL OPERATORS
           ------------------------------------- */

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
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_GT"
                : "CMP_GT");

        break;

    case TAC_LESS_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_LE"
                : "CMP_LE");

        break;

    case TAC_GREATER_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_GE"
                : "CMP_GE");

        break;

        /* -------------------------------------
           EQUALITY OPERATORS
           ------------------------------------- */

    case TAC_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_EQ"
                : "CMP_EQ");

        break;

    case TAC_NOT_EQUAL:

        generate_binary_operation(
            generator,
            instruction,
            instruction->operand1_type == AST_TYPE_FLOAT
                ? "FCMP_NE"
                : "CMP_NE");

        break;

        /* -------------------------------------
           UNARY OPERATORS
           ------------------------------------- */

    case TAC_NEGATE:

        generate_unary_operation(
            generator,
            instruction,
            instruction->result_type == AST_TYPE_FLOAT
                ? "FNEG"
                : "NEG");

        break;

    case TAC_NOT:

        generate_unary_operation(
            generator,
            instruction,
            "NOT");

        break;

        /* -------------------------------------
           TYPE CONVERSION
           ------------------------------------- */

    case TAC_INT_TO_FLOAT:
    {
        /*
         * Load the integer operand.
         */
        int source_register = load_operand(
            generator,
            instruction->operand1,
            instruction->operand1_type);

        if (source_register < 0)
        {
            break;
        }

        /*
         * Protect the source register.
         */
        register_protect(generator, source_register);

        /*
         * Allocate a destination register.
         */
        int result_register = register_acquire(generator);

        if (result_register < 0)
        {
            generator->failed = 1;

            register_unprotect(generator, source_register);

            break;
        }

        /*
         * Generate integer-to-float conversion.
         */
        fprintf(
            output,
            "    ITOF %s, %s\n",
            register_name(result_register),
            register_name(source_register));

        /*
         * Store the converted result.
         */
        store_result(
            generator,
            instruction->result,
            result_register);

        /*
         * Release the source register.
         */
        register_unprotect(generator, source_register);

        break;
    }

        /* -------------------------------------
           CONTROL FLOW
           ------------------------------------- */

    case TAC_LABEL:

        if (instruction->result == NULL)
        {
            generator->failed = 1;
            break;
        }

        /*
         * A label can be reached from different
         * control-flow paths. Invalidate cached values.
         */
        register_clear_all(generator);

        fprintf(
            output,
            "%s:\n",
            instruction->result);

        break;

    case TAC_GOTO:

        if (instruction->result == NULL)
        {
            generator->failed = 1;
            break;
        }

        fprintf(
            output,
            "    JMP %s\n",
            instruction->result);

        register_clear_all(generator);

        break;

    case TAC_IF_FALSE_GOTO:
    {
        int condition_register = load_operand(
            generator,
            instruction->operand1,
            instruction->operand1_type);

        if (condition_register < 0)
        {
            break;
        }

        if (instruction->result == NULL)
        {
            generator->failed = 1;
            break;
        }

        fprintf(
            output,
            "    JZ %s, %s\n",
            register_name(condition_register),
            instruction->result);

        /*
         * Both possible execution paths must reload
         * values from memory rather than relying on
         * cached register contents.
         */
        register_clear_all(generator);

        break;
    }

        /* -------------------------------------
           OPTIMIZED-AWAY INSTRUCTIONS
           ------------------------------------- */

    case TAC_NOP:

        /*
         * NOP instructions are retained in the TAC
         * structure but do not generate assembly.
         */

        break;

        /* -------------------------------------
           UNSUPPORTED OPERATIONS
           ------------------------------------- */

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
        perror(
            "CODEGEN ERROR: Unable to create output file");

        return 0;
    }

    CodeGenerator generator;

    generator.output = output;
    generator.failed = 0;
    generator.register_clock = 0;

    frame_init(&generator.frame);

    /*
     * Initially, all physical registers are available.
     */
    for (int i = 0; i < CODEGEN_REGISTER_COUNT; i++)
    {
        generator.registers[i].value_name = NULL;
        generator.registers[i].last_used = 0;
        generator.registers[i].valid = 0;
        generator.registers[i].protected = 0;
    }

    /*
     * Construct activation frame.
     */

    if (!build_activation_frame(
            &generator.frame,
            program))
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Unable to construct activation frame.\n");

        generator.failed = 1;
    }

    /*
     * Generate target assembly.
     */

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

        if (!generator.failed)
        {
            fprintf(
                output,
                "\n    HALT\n");
        }
    }

    /*
     * Check file output errors.
     */

    if (ferror(output))
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Failed while writing target assembly.\n");

        generator.failed = 1;
    }

    /*
     * Release memory.
     */

    register_clear_all(&generator);

    frame_destroy(&generator.frame);

    if (fclose(output) != 0)
    {
        fprintf(
            stderr,
            "CODEGEN ERROR: Unable to close output file.\n");

        generator.failed = 1;
    }

    /*
     * Remove incomplete output if generation failed.
     */

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

/*
 * Print an already-generated assembly file.
 */

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