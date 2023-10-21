#pragma once

#include <common.h>
#include <sv/sv.h>

typedef enum {
    TOK_EOF,
    TOK_GARBAGE,

    TOK_DEF,
    TOK_LET,
    TOK_RETURN,

    TOK_IDENTIFIER,
    TOK_INTLITERAL,
    TOK_FLOATLITERAL,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LCURLY,
    TOK_RCURLY,

    TOK_COLON,
    TOK_COMMA,
    TOK_SEMICOLON,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
} token_kind_t;

const char* token_kind_to_str(token_kind_t);

typedef struct {
    token_kind_t kind;
    sv_t span;
    location_t location;
} token_t;

token_t token_make(token_kind_t, sv_t, location_t);
