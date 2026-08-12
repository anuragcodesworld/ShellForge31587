#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

#define INITIAL_CAPACITY 10

static void add_token(TokenList *list, TokenType type, const char *value)
{
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;

        Token *temp = realloc(
            list->tokens,
            list->capacity * sizeof(Token)
        );

        if (temp == NULL)
        {
            perror("realloc");
            free_tokens(list);
            exit(EXIT_FAILURE);
        }

        list->tokens = temp;
    }

    list->tokens[list->count].type = type;

    if (value != NULL)
    {
        list->tokens[list->count].value = malloc(strlen(value) + 1);

        if (list->tokens[list->count].value == NULL)
        {
            perror("malloc");
            free_tokens(list);
            exit(EXIT_FAILURE);
        }

        strcpy(list->tokens[list->count].value, value);
    }
    else
    {
        list->tokens[list->count].value = NULL;
    }

    list->count++;
}

TokenList *tokenize(const char *input)
{
    TokenList *list = malloc(sizeof(TokenList));

    if (list == NULL)
    {
        perror("malloc");
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_CAPACITY;

    list->tokens = malloc(
        list->capacity * sizeof(Token)
    );

    if (list->tokens == NULL)
    {
        perror("malloc");
        free(list);
        return NULL;
    }

    int i = 0;

    while (input[i] != '\0')
    {
        /* Skip spaces and tabs */
        if (input[i] == ' ' || input[i] == '\t')
        {
            i++;
            continue;
        }

        /* Pipe */
        if (input[i] == '|')
        {
            add_token(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirection */
        if (input[i] == '<')
        {
            add_token(list, TOKEN_REDIRECT_IN, "<");
            i++;
            continue;
        }

        /* Output redirection / append */
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                add_token(list, TOKEN_APPEND, ">>");
                i += 2;
            }
            else
            {
                add_token(list, TOKEN_REDIRECT_OUT, ">");
                i++;
            }

            continue;
        }

        /* Normal word */
        int start = i;

        while (input[i] != '\0' &&
               input[i] != ' ' &&
               input[i] != '\t' &&
               input[i] != '|' &&
               input[i] != '<' &&
               input[i] != '>')
        {
            i++;
        }

        int length = i - start;

        char *word = malloc(length + 1);

        if (word == NULL)
        {
            perror("malloc");
            free_tokens(list);
            exit(EXIT_FAILURE);
        }

        strncpy(word, &input[start], length);
        word[length] = '\0';

        add_token(list, TOKEN_WORD, word);

        free(word);
    }

    /* End token */
    add_token(list, TOKEN_END, NULL);

    return list;
}

void free_tokens(TokenList *list)
{
    if (list == NULL)
        return;

    for (int i = 0; i < list->count; i++)
    {
        free(list->tokens[i].value);
    }

    free(list->tokens);
    free(list);
}
