#include <stdio.h>
#include <stdlib.h>

#include "../ast/ast.h"

/* Functions and variables provided by Flex and Bison. */
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

    /* Read from a file if supplied; otherwise use standard input. */
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

    int result = yyparse();

    if (argc == 2)
    {
        fclose(yyin);
    }

    if (result == 0 && ast_root != NULL)
    {
        printf("\nParsing successful!\n");
        printf("Abstract Syntax Tree constructed successfully.\n");

        ast_free(ast_root);
        ast_root = NULL;

        printf("\nCompilation frontend completed successfully.\n");

        return EXIT_SUCCESS;
    }

    fprintf(stderr, "\nCompilation failed.\n");

    ast_free(ast_root);
    ast_root = NULL;

    return EXIT_FAILURE;
}