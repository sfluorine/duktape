#include <token.h>

const char* token_kind_to_str(token_kind_t kind) {
    switch (kind) {
        case TOK_EOF:
            return "end of file";
        case TOK_GARBAGE:
            return "garbage";

        case TOK_DEF:
            return "def";
        case TOK_LET:
            return "let";
        case TOK_RETURN:
            return "return";

        case TOK_IDENTIFIER:
            return "identifier";
        case TOK_INTLITERAL:
            return "int literal";
        case TOK_FLOATLITERAL:
            return "float literal";

        case TOK_LPAREN:
            return "(";
        case TOK_RPAREN:
            return ")";
        case TOK_LCURLY:
            return "{";
        case TOK_RCURLY:
            return "}";

        case TOK_COLON:
            return ":";
        case TOK_COMMA:
            return ",";
        case TOK_SEMICOLON:
            return ";";

        case TOK_PLUS:
            return "+";
        case TOK_MINUS:
            return "-";
        case TOK_STAR:
            return "*";
        case TOK_SLASH:
            return "/";
    }
}
