#ifndef THREE_ADDRESS_CODE_H
#define THREE_ADDRESS_CODE_H

#include "../ast/ast.h"

/* Supported TAC instruction operations */
typedef enum
{
    TAC_ASSIGN,

    TAC_ADD,
    TAC_SUBTRACT,
    TAC_MULTIPLY,
    TAC_DIVIDE,

    TAC_LESS,
    TAC_GREATER,
    TAC_LESS_EQUAL,
    TAC_GREATER_EQUAL,

    TAC_EQUAL,
    TAC_NOT_EQUAL,

    TAC_AND,
    TAC_OR,

    TAC_NEGATE,
    TAC_NOT,

    TAC_INT_TO_FLOAT,

    TAC_LABEL,
    TAC_GOTO,
    TAC_IF_FALSE_GOTO,

    TAC_NOP
} TACOperation;

/* One three-address instruction */
typedef struct TACInstruction
{
    TACOperation operation;

    char *result;
    char *operand1;
    char *operand2;

    /*
     * Type information used by the backend.
     *
     * For operations without a value result, such as
     * labels and jumps, these fields are ignored.
     */
    ASTDataType result_type;
    ASTDataType operand1_type;
    ASTDataType operand2_type;

    struct TACInstruction *next;
} TACInstruction;

/* Complete intermediate-code program */
typedef struct
{
    TACInstruction *head;
    TACInstruction *tail;

    unsigned int temporary_count;
    unsigned int label_count;
} TACProgram;

/* Generate TAC from an AST */
TACProgram *tac_generate(ASTNode *root);

/* Display generated TAC */
void tac_print(const TACProgram *program);

/* Release TAC memory */
void tac_free(TACProgram *program);

#endif
