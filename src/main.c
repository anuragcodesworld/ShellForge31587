#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"

int main(void)
{
    printf("=====================================\n");
    printf("            Shellforge\n");
    printf("     A Unix Style Shell written in C\n");
    printf("=====================================\n");

    while (1)
    {
        char *line = readline("shellforge$ ");

        /* Ctrl + D */
        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        /* Ignore empty input */
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        /* Add command to history */
        add_history(line);

        /*
         * HISTORY
         */
        if (strcmp(line, "history") == 0)
        {
            HIST_ENTRY **hist = history_list();

            if (hist != NULL)
            {
                for (int i = 0; hist[i] != NULL; i++)
                {
                    printf("%d  %s\n", i + 1, hist[i]->line);
                }
            }

            free(line);
            continue;
        }

        /*
         * STEP 1
         * LEXER / TOKENIZER
         */
        TokenList *tokens = lex(line);

        if (tokens == NULL)
        {
            free(line);
            continue;
        }

        /*
         * Display tokens
         */
        printf("\n-------------- TOKENS --------------\n");

        for (int i = 0; i < tokens->count; i++)
        {
            Token *token = &tokens->tokens[i];

            switch (token->type)
            {
                case TOKEN_WORD:
                    printf("%d : WORD          %s\n",
                           i, token->value);
                    break;

                case TOKEN_PIPE:
                    printf("%d : PIPE          %s\n",
                           i, token->value);
                    break;

                case TOKEN_REDIRECT_IN:
                    printf("%d : REDIRECT_IN   %s\n",
                           i, token->value);
                    break;

                case TOKEN_REDIRECT_OUT:
                    printf("%d : REDIRECT_OUT  %s\n",
                           i, token->value);
                    break;

                case TOKEN_APPEND:
                    printf("%d : APPEND        %s\n",
                           i, token->value);
                    break;

                case TOKEN_END:
                    printf("%d : END           END\n",
                           i);
                    break;
            }
        }

        printf("------------------------------------\n");

        /*
         * STEP 2
         * VARIABLE EXPANSION
         */
        expand_tokens(tokens);

        /*
         * STEP 3
         * PARSER
         */
        Pipeline *pipeline = parse(tokens);

        if (pipeline == NULL)
        {
            free_tokens(tokens);
            free(line);
            continue;
        }

        /*
         * Display parsed pipeline
         */
        print_pipeline(pipeline);

        /*
         * STEP 4
         * EXECUTION
         */
        int should_exit = 0;

        for (int i = 0; i < pipeline->count; i++)
        {
            command_t *cmd = &pipeline->commands[i];

            if (cmd->argc == 0)
            {
                continue;
            }

            /*
             * Execute builtin or external command.
             */
            int status = execute_command(cmd);

            /*
             * builtin_exit() returns 1.
             */
            if (status == 1 &&
                strcmp(cmd->argv[0], "exit") == 0)
            {
                should_exit = 1;
                break;
            }
        }

        /*
         * STEP 5
         * CLEANUP
         */
        free_pipeline(pipeline);
        free_tokens(tokens);
        free(line);

        /*
         * Exit the shell after cleanup.
         */
        if (should_exit)
        {
            printf("Exiting...\n");
            break;
        }
    }

    return 0;
}
