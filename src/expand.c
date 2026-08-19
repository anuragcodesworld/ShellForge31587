#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"

static char *expand_variable(const char *value)
{
    if (value == NULL)
    {
        return NULL;
    }

    /* Only expand values beginning with '$' */
    if (value[0] != '$')
    {
        char *result = malloc(strlen(value) + 1);

        if (result == NULL)
        {
            perror("malloc");
            return NULL;
        }

        strcpy(result, value);
        return result;
    }

    const char *name = value + 1;

    if (*name == '\0')
    {
        return NULL;
    }

    const char *environment_value = getenv(name);

    if (environment_value == NULL)
    {
        return NULL;
    }

    char *result = malloc(strlen(environment_value) + 1);

    if (result == NULL)
    {
        perror("malloc");
        return NULL;
    }

    strcpy(result, environment_value);

    return result;
}

void expand_tokens(TokenList *tokens)
{
    if (tokens == NULL)
    {
        return;
    }

    for (int i = 0; i < tokens->count; i++)
    {
        Token *token = &tokens->tokens[i];

        if (token->type != TOKEN_WORD)
        {
            continue;
        }

        if (token->value == NULL || token->value[0] != '$')
        {
            continue;
        }

        char *expanded = expand_variable(token->value);

        if (expanded != NULL)
        {
            free(token->value);
            token->value = expanded;
        }
    }
}
