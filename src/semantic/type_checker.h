#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "../ast/ast.h"
#include "symbol_table.h"

typedef struct
{
    SymbolTable symbol_table;
    int error_count;
} TypeChecker;

void type_checker_init(TypeChecker *checker);
void type_checker_destroy(TypeChecker *checker);

int type_check_program(TypeChecker *checker, ASTNode *program);

#endif