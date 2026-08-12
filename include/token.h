#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIRECT_IN,
    TOKEN_REDIRECT_OUT,
    TOKEN_APPEND,
    TOKEN_END
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

TokenList *tokenize(const char *input);
void free_tokens(TokenList *list);

#endif
