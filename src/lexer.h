#pragma once

#include <token.h>

typedef struct {
    char* input;
    int cursor;
    int line;
    int col;
} lexer_t;

void lexer_init(lexer_t*, char*);
void lexer_deinit(lexer_t*);

token_t* get_tokens(lexer_t*);
