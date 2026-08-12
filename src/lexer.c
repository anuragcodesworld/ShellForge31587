#include <stdio.h>
#include "lexer.h"

TokenList *lex(const char *input)
{
    if (input == NULL) {
        return NULL;
    }

    return tokenize(input);
}
