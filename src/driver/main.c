#include <stdio.h>
#include <stdlib.h>

#include "../ast/ast.h"
#include "../semantic/type_checker.h"
#include "../ir/three_address_code.h"

#include "../optimiser/optimizer.h"

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *ast_root;

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [source_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2)
    {
        yyin = fopen(argv[1], "r");

        if (yyin == NULL)
        {
            perror("Unable to open source file");
            return EXIT_FAILURE;
        }
    }

    printf("=================================\n");
    printf("       TYPED-MINI COMPILER\n");
    printf("=================================\n");

    printf("\nStarting lexical and syntax analysis...\n");

    int parse_result = yyparse();

    if (argc == 2)
    {
        fclose(yyin);
    }

    if (parse_result != 0 || ast_root == NULL)
    {
        fprintf(stderr, "\nCompilation failed during parsing.\n");
        ast_free(ast_root);
        ast_root = NULL;
        return EXIT_FAILURE;
    }

    printf("Syntax analysis successful.\n");

    printf("\nStarting semantic analysis...\n");

    TypeChecker checker;

    type_checker_init(&checker);

    int semantic_result = type_check_program(&checker, ast_root);

    type_checker_destroy(&checker);

    if (!semantic_result)
    {
        ast_free(ast_root);
        ast_root = NULL;

        fprintf(stderr, "\nCompilation failed during semantic analysis.\n");
        return EXIT_FAILURE;
    }

    printf("\nStarting intermediate code generation...\n");

    TACProgram *tac_program = tac_generate(ast_root);

    if (tac_program == NULL)
    {
        ast_free(ast_root);
        ast_root = NULL;

        fprintf(stderr, "\nCompilation failed during TAC generation.\n");
        return EXIT_FAILURE;
    }

    printf("Intermediate code generation successful.\n");

    tac_print(tac_program);

    printf("\nStarting control-flow graph construction...\n");

    ControlFlowGraph *cfg = optimizer_build_cfg(tac_program);

    if (cfg == NULL)
    {
        tac_free(tac_program);
        ast_free(ast_root);
        ast_root = NULL;

        fprintf(stderr, "\nCompilation failed during CFG construction.\n");
        return EXIT_FAILURE;
    }

    printf("Control-flow graph construction successful.\n");

    printf("\n========== BEFORE OPTIMISATION ==========\n");
    optimizer_print_cfg(cfg);

    optimizer_run(cfg);

    printf("\n========== AFTER OPTIMISATION ==========\n");
    optimizer_print_cfg(cfg);

    optimizer_free_cfg(cfg);
    tac_free(tac_program);

    ast_free(ast_root);
    ast_root = NULL;

    printf("\n=================================\n");
    printf("COMPILATION FRONTEND SUCCESSFUL!\n");
    printf("=================================\n");

    return EXIT_SUCCESS;
}
