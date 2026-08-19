#ifndef PARSER_H
#define PARSER_H

#include "token.h"

#define MAX_ARGS 64
#define MAX_COMMANDS 16

typedef struct
{
    char *argv[MAX_ARGS];
    int argc;

    char *input;
    char *output;

    int append;
    int background;
} Command;

typedef struct
{
    Command commands[MAX_COMMANDS];
    int count;
} Pipeline;

Pipeline *parse(TokenList *tokens);
void free_pipeline(Pipeline *pipeline);
void print_pipeline(Pipeline *pipeline);

#endif
