#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "lexer.h"
#include "token.h"

int main(void)
{
    printf("=====================================\n");
    printf("        Welcome to Shellforge\n");
    printf("=====================================\n");

    while (1)
    {
        char *line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        add_history(line);

        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exiting...\n");
            break;
        }

        /* Send the command to the lexer */
        TokenList *tokens = lex(line);

        if (tokens == NULL)
        {
            free(line);
            continue;
        }

        printf("\nTokens:\n");

        for (int i = 0; i < tokens->count; i++)
        {
            Token *token = &tokens->tokens[i];

            switch (token->type)
            {
                case TOKEN_WORD:
                    printf("WORD         : %s\n", token->value);
                    break;

                case TOKEN_PIPE:
                    printf("PIPE         : %s\n", token->value);
                    break;

                case TOKEN_REDIRECT_IN:
                    printf("REDIRECT_IN  : %s\n", token->value);
                    break;

                case TOKEN_REDIRECT_OUT:
                    printf("REDIRECT_OUT : %s\n", token->value);
                    break;

                case TOKEN_APPEND:
                    printf("APPEND       : %s\n", token->value);
                    break;

                case TOKEN_END:
                    printf("END\n");
                    break;
            }
        }

        printf("\n");

        free_tokens(tokens);
        free(line);
    }

    return 0;
}
