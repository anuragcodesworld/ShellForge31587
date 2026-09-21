#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"


static char *copy_string(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    char *copy = malloc(strlen(str) + 1);

    if (copy == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(copy, str);

    return copy;
}


static void initialize_command(Command *command)
{
    command->argc = 0;
    command->input = NULL;
    command->output = NULL;
    command->append = 0;
    command->background = 0;

    for (int i = 0; i < MAX_ARGS; i++)
    {
        command->argv[i] = NULL;
    }
}


Pipeline *parse(TokenList *tokens)
{
    if (tokens == NULL)
    {
        return NULL;
    }

    Pipeline *pipeline = malloc(sizeof(Pipeline));

    if (pipeline == NULL)
    {
        perror("malloc");
        return NULL;
    }

    pipeline->count = 1;

    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        initialize_command(&pipeline->commands[i]);
    }

    int command_index = 0;

    for (int i = 0; i < tokens->count; i++)
    {
        Token *token = &tokens->tokens[i];
        Command *command = &pipeline->commands[command_index];

        /* End of input */
        if (token->type == TOKEN_END)
        {
            break;
        }


        /* Pipe */
        if (token->type == TOKEN_PIPE)
        {
            if (command_index + 1 >= MAX_COMMANDS)
            {
                fprintf(stderr,
                        "Too many commands in pipeline\n");

                free_pipeline(pipeline);
                return NULL;
            }

            command_index++;
            pipeline->count++;

            continue;
        }


        /* Background */
        if (token->type == TOKEN_BACKGROUND)
        {
            command->background = 1;
            continue;
        }


        /* Input redirection */
        if (token->type == TOKEN_REDIRECT_IN)
        {
            if (i + 1 < tokens->count &&
                tokens->tokens[i + 1].type == TOKEN_WORD)
            {
                i++;

                command->input =
                    copy_string(tokens->tokens[i].value);
            }

            continue;
        }


        /* Output redirection */
        if (token->type == TOKEN_REDIRECT_OUT)
        {
            if (i + 1 < tokens->count &&
                tokens->tokens[i + 1].type == TOKEN_WORD)
            {
                i++;

                command->output =
                    copy_string(tokens->tokens[i].value);

                command->append = 0;
            }

            continue;
        }


        /* Append redirection */
        if (token->type == TOKEN_APPEND)
        {
            if (i + 1 < tokens->count &&
                tokens->tokens[i + 1].type == TOKEN_WORD)
            {
                i++;

                command->output =
                    copy_string(tokens->tokens[i].value);

                command->append = 1;
            }

            continue;
        }


        /* Normal argument */
        if (token->type == TOKEN_WORD)
        {
            if (command->argc < MAX_ARGS - 1)
            {
                command->argv[command->argc] =
                    copy_string(token->value);

                command->argc++;

                command->argv[command->argc] = NULL;
            }

            continue;
        }
    }

    return pipeline;
}


void free_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL)
    {
        return;
    }

    for (int i = 0; i < pipeline->count; i++)
    {
        Command *command = &pipeline->commands[i];

        for (int j = 0; j < command->argc; j++)
        {
            free(command->argv[j]);
        }

        free(command->input);
        free(command->output);
    }

    free(pipeline);
}


void print_pipeline(Pipeline *pipeline)
{
    if (pipeline == NULL)
    {
        return;
    }

    printf("\n========== PIPELINE ==========\n\n");

    for (int i = 0; i < pipeline->count; i++)
    {
        Command *command = &pipeline->commands[i];

        printf("Command %d\n", i + 1);
        printf("------------------------------\n");

        printf("Arguments\n");

        for (int j = 0; j < command->argc; j++)
        {
            printf("argv[%d] = %s\n",
                   j,
                   command->argv[j]);
        }

        printf("Input      : %s\n",
               command->input != NULL
                   ? command->input
                   : "None");

        printf("Output     : %s\n",
               command->output != NULL
                   ? command->output
                   : "None");

        printf("Append     : %s\n",
               command->append ? "Yes" : "No");

        printf("Background : %s\n",
               command->background ? "Yes" : "No");

        printf("==============================\n\n");
    }
}
