#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "../ast/ast.h"

typedef struct Symbol
{
    char *name;
    ASTDataType data_type;
    struct Symbol *next;
} Symbol;

typedef struct Scope
{
    Symbol *symbols;
    struct Scope *parent;
} Scope;

typedef struct
{
    Scope *current_scope;
} SymbolTable;

/* Symbol table management */
void symbol_table_init(SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

/* Scope management */
void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_exit_scope(SymbolTable *table);

/* Symbol operations */
int symbol_table_declare(
    SymbolTable *table,
    const char *name,
    ASTDataType data_type);

Symbol *symbol_table_lookup(
    SymbolTable *table,
    const char *name);

#endif