#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *duplicate_string(const char *text)
{
    size_t length = strlen(text) + 1;

    char *copy = malloc(length);

    if (copy == NULL)
    {
        perror("Symbol table allocation failed");
        exit(EXIT_FAILURE);
    }

    memcpy(copy, text, length);

    return copy;
}

void symbol_table_enter_scope(SymbolTable *table)
{
    Scope *scope = calloc(1, sizeof(Scope));

    if (scope == NULL)
    {
        perror("Scope allocation failed");
        exit(EXIT_FAILURE);
    }

    scope->parent = table->current_scope;
    table->current_scope = scope;
}

void symbol_table_exit_scope(SymbolTable *table)
{
    if (table == NULL || table->current_scope == NULL)
    {
        return;
    }

    Scope *scope = table->current_scope;
    Symbol *symbol = scope->symbols;

    while (symbol != NULL)
    {
        Symbol *next = symbol->next;

        free(symbol->name);
        free(symbol);

        symbol = next;
    }

    table->current_scope = scope->parent;

    free(scope);
}

void symbol_table_init(SymbolTable *table)
{
    table->current_scope = NULL;

    symbol_table_enter_scope(table);
}

void symbol_table_destroy(SymbolTable *table)
{
    while (table->current_scope != NULL)
    {
        symbol_table_exit_scope(table);
    }
}

int symbol_table_declare(
    SymbolTable *table,
    const char *name,
    ASTDataType data_type)
{
    if (table == NULL || table->current_scope == NULL)
    {
        return 0;
    }

    /* Prevent duplicate declarations in the same scope. */
    Symbol *current = table->current_scope->symbols;

    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return 0;
        }

        current = current->next;
    }

    Symbol *symbol = malloc(sizeof(Symbol));

    if (symbol == NULL)
    {
        perror("Symbol allocation failed");
        exit(EXIT_FAILURE);
    }

    symbol->name = duplicate_string(name);
    symbol->data_type = data_type;

    symbol->next = table->current_scope->symbols;

    table->current_scope->symbols = symbol;

    return 1;
}

Symbol *symbol_table_lookup(
    SymbolTable *table,
    const char *name)
{
    if (table == NULL)
    {
        return NULL;
    }

    Scope *scope = table->current_scope;

    /* Search current scope and then parent scopes. */
    while (scope != NULL)
    {
        Symbol *symbol = scope->symbols;

        while (symbol != NULL)
        {
            if (strcmp(symbol->name, name) == 0)
            {
                return symbol;
            }

            symbol = symbol->next;
        }

        scope = scope->parent;
    }

    return NULL;
}