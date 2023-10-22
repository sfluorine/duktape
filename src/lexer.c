#include <ctype.h>
#include <dynarray/dynarray.h>
#include <lexer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static char current(lexer_t* lexer) {
    return lexer->input[lexer->cursor];
}

static bool is_eof(lexer_t* lexer) {
    return lexer->input == NULL || current(lexer) == 0;
}

static void advance(lexer_t* lexer) {
    if (is_eof(lexer)) {
        return;
    }

    if (current(lexer) == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }

    lexer->cursor++;
}

static void skip_ws_and_comments(lexer_t* lexer) {
    while (!is_eof(lexer) && (isspace(current(lexer)) || current(lexer) == '#')) {
        while (!is_eof(lexer) && isspace(current(lexer))) {
            advance(lexer);
        }

        if (current(lexer) == '#') {
            do {
                advance(lexer);
            } while (!is_eof(lexer) && current(lexer) != '\n');
        }
    }
}

void lexer_init(lexer_t* lexer, char* input) {
    lexer->input  = input;
    lexer->cursor = 0;
    lexer->line   = 1;
    lexer->col    = 1;
}

void lexer_deinit(lexer_t* lexer) {
    if (lexer->input) {
        free(lexer->input);
    }
}

token_t* get_tokens(lexer_t* lexer) {
    token_t* tokens = dynarray_create(token_t);

    while (true) {
        skip_ws_and_comments(lexer);

        const char* start = &lexer->input[lexer->cursor];
        const int start_line = lexer->line;
        const int start_col  = lexer->col;

        if (is_eof(lexer)) {
            dynarray_push_rval(tokens, token_make(TOK_EOF, sv_make_from(""), location_make(start_line, start_col)));
            break;
        }

        switch (current(lexer))  {
            case '(':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_LPAREN, sv_make_from("("), location_make(start_line, start_col)));
                continue;
            case ')':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_RPAREN, sv_make_from(")"), location_make(start_line, start_col)));
                continue;
            case '{':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_LCURLY, sv_make_from("{"), location_make(start_line, start_col)));
                continue;
            case '}':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_RCURLY, sv_make_from("}"), location_make(start_line, start_col)));
                continue;
            case '=':
                advance(lexer);
                if (current(lexer) == '=') {
                    advance(lexer);

                    dynarray_push_rval(tokens, token_make(TOK_EQUAL_EQUAL, sv_make_from("=="), location_make(start_line, start_col)));
                    continue;
                }
                dynarray_push_rval(tokens, token_make(TOK_EQUAL, sv_make_from("="), location_make(start_line, start_col)));
                continue;
            case ':':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_COLON, sv_make_from(":"), location_make(start_line, start_col)));
                continue;
            case ',':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_COMMA, sv_make_from(","), location_make(start_line, start_col)));
                continue;
            case ';':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_SEMICOLON, sv_make_from(";"), location_make(start_line, start_col)));
                continue;
            case '!':
                advance(lexer);
                if (current(lexer) == '=') {
                    advance(lexer);

                    dynarray_push_rval(tokens, token_make(TOK_BANG_EQUAL, sv_make_from("!="), location_make(start_line, start_col)));
                    continue;
                }
                dynarray_push_rval(tokens, token_make(TOK_BANG, sv_make_from("!"), location_make(start_line, start_col)));
                continue;
            case '+':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_PLUS, sv_make_from("+"), location_make(start_line, start_col)));
                continue;
            case '-':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_MINUS, sv_make_from("-"), location_make(start_line, start_col)));
                continue;
            case '*':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_STAR, sv_make_from("*"), location_make(start_line, start_col)));
                continue;
            case '/':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_SLASH, sv_make_from("/"), location_make(start_line, start_col)));
                continue;
            case '<':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_LESS, sv_make_from("<"), location_make(start_line, start_col)));
                continue;
            case '>':
                advance(lexer);
                dynarray_push_rval(tokens, token_make(TOK_GREATER, sv_make_from(">"), location_make(start_line, start_col)));
                continue;
            default:
                break;
        }

        if (isdigit(current(lexer))) {
            int len = 0;
            do {
                len++;
                advance(lexer);
            } while (!is_eof(lexer) && isdigit(current(lexer)));

            if (current(lexer) == '.') {
                len++;
                advance(lexer);

                int mantissa = 0;
                while (!is_eof(lexer) && isdigit(current(lexer))) {
                    mantissa++;
                    advance(lexer);
                }

                if (mantissa == 0) {
                    fprintf(stderr, 
                            LOCATION_FMT" WARNING: invalid floating point will result to garbage token.\n",
                            LOCATION_ARG(location_make(start_line, start_col)));

                    dynarray_push_rval(tokens, token_make(TOK_GARBAGE, sv_make(start, len + mantissa), location_make(start_line, start_col)));
                    continue;
                }

                dynarray_push_rval(tokens, token_make(TOK_FLOATLITERAL, sv_make(start, len + mantissa), location_make(start_line, start_col)));
                continue;
            }

            dynarray_push_rval(tokens, token_make(TOK_INTLITERAL, sv_make(start, len), location_make(start_line, start_col)));
            continue;
        }

        if (isalpha(current(lexer)) || current(lexer) == '_') {
            int len = 0;
            do {
                len++;
                advance(lexer);
            } while (!is_eof(lexer) && (isalnum(current(lexer)) || current(lexer) == '_'));

            sv_t span = sv_make(start, len);

            if (sv_equals(span, sv_make_from("def"))) {
                dynarray_push_rval(tokens, token_make(TOK_DEF, sv_make_from("def"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("let"))) {
                dynarray_push_rval(tokens, token_make(TOK_LET, sv_make_from("let"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("return"))) {
                dynarray_push_rval(tokens, token_make(TOK_RETURN, sv_make_from("return"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("or"))) {
                dynarray_push_rval(tokens, token_make(TOK_OR, sv_make_from("or"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("and"))) {
                dynarray_push_rval(tokens, token_make(TOK_AND, sv_make_from("and"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("true"))) {
                dynarray_push_rval(tokens, token_make(TOK_TRUE, sv_make_from("true"), location_make(start_line, start_col)));
                continue;
            } else if (sv_equals(span, sv_make_from("false"))) {
                dynarray_push_rval(tokens, token_make(TOK_FALSE, sv_make_from("false"), location_make(start_line, start_col)));
                continue;
            }else {
                dynarray_push_rval(tokens, token_make(TOK_IDENTIFIER, sv_make(start, len), location_make(start_line, start_col)));
                continue;
            }
        }

        int len = 0;
        do {
            len++;
            advance(lexer);
        } while (!is_eof(lexer) && !isspace(current(lexer)));

        sv_t span = sv_make(start, len);

        fprintf(stderr, LOCATION_FMT" WARNING: garbage token: "SV_FMT"\n", LOCATION_ARG(location_make(start_line, start_col)), SV_ARG(span));
        dynarray_push_rval(tokens, token_make(TOK_GARBAGE, span, location_make(start_line, start_col)));
        continue;
    }

    return tokens;
}
