#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../ir/three_address_code.h"

/* A basic block contains a consecutive sequence of TAC instructions. */
typedef struct BasicBlock
{
    unsigned int id;

    TACInstruction *first;
    TACInstruction *last;

    struct BasicBlock *successors[2];
    unsigned int successor_count;

    struct BasicBlock *next;
} BasicBlock;

/* Control-flow graph */
typedef struct
{
    BasicBlock *entry;
    BasicBlock *blocks;

    unsigned int block_count;
} ControlFlowGraph;

/* Build basic blocks from TAC */
ControlFlowGraph *optimizer_build_cfg(TACProgram *program);

/* Display the control-flow graph */
void optimizer_print_cfg(const ControlFlowGraph *cfg);

/* Release CFG memory */
void optimizer_free_cfg(ControlFlowGraph *cfg);

/* Optimisation passes */
void optimizer_constant_folding(ControlFlowGraph *cfg);
void optimizer_copy_propagation(ControlFlowGraph *cfg);

/* Run all optimisation passes */
void optimizer_run(ControlFlowGraph *cfg);

#endif