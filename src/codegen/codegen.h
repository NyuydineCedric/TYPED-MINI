#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

#include "../ir/three_address_code.h"

/*
 * TYPED-MINI virtual machine register set.
 *
 * R0 and R1: operand registers.
 * R2: result register.
 */
#define CODEGEN_REGISTER_COUNT 3

/*
 * Each variable and temporary occupies
 * one 8-byte memory slot.
 */
#define CODEGEN_SLOT_SIZE 8

/*
 * Generate target assembly from TAC.
 *
 * program:
 *     Optimized or unoptimized TAC program.
 *
 * output_path:
 *     Destination assembly file.
 *
 * Returns:
 *     1 when generation succeeds.
 *     0 when generation fails.
 */
int codegen_generate(
    const TACProgram *program,
    const char *output_path);

/*
 * Print generated target instructions.
 */
void codegen_print(FILE *output);

#endif