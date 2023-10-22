#include <dynarray/dynarray.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>

static token_t current(parser_t* parser) {
    return parser->tokens[parser->cursor];
}

static bool is_eof(parser_t* parser) {
    return current(parser).kind == TOK_EOF;
}

static bool expect(parser_t* parser, token_kind_t kind) {
    if (is_eof(parser)) {
        return false;
    }

    return current(parser).kind == kind;
}

static void advance(parser_t* parser) {
    if (is_eof(parser)) {
        return;
    }

    parser->cursor++;
}

static void match(parser_t* parser, token_kind_t kind) {
    if (!expect(parser, kind)) {
        fprintf(stderr, LOCATION_FMT" ERROR: expected: %s but got "SV_FMT"\n", LOCATION_ARG(current(parser).location), token_kind_to_str(kind), SV_ARG(current(parser).span));
        exit(EXIT_FAILURE);
    }

    advance(parser);
}

void parser_init(parser_t* parser, token_t* tokens) {
    parser->tokens = tokens;
    parser->cursor = 0;
}

void parser_deinit(parser_t* parser) {
    dynarray_destroy(parser->tokens);
}

expression_t* parse_primary(parser_t* parser) {
    location_t location = current(parser).location;

    if (expect(parser, TOK_LPAREN)) {
        advance(parser);

        expression_t* expr = parse_expression(parser);

        match(parser, TOK_RPAREN);

        return expr;
    } else if (expect(parser, TOK_IDENTIFIER)) {
        token_t id = current(parser);
        advance(parser);

        if (expect(parser, TOK_LPAREN)) {
            advance(parser);

            funcall_t* funcall = funcall_make(id.span, location);

            bool first = true;
            while (!is_eof(parser) && !expect(parser, TOK_RPAREN)) {
                if (!first) {
                    match(parser, TOK_COMMA);
                }

                dynarray_push_rval(funcall->arguments, parse_expression(parser));
                first = false;
            }

            match(parser, TOK_RPAREN);

            expression_t* expr = expression_make(EXPR_PRIMARY, location);
            primary_t* primary = primary_make(PRIMARY_FUNCALL, location);
            primary->as.funcall = funcall;
            expr->as.primary = primary;

            return expr;
        }

        expression_t* expr = expression_make(EXPR_PRIMARY, location);
        primary_t* primary = primary_make(PRIMARY_IDENTIFIER, location);
        primary->as.identifier = id.span;
        expr->as.primary = primary;

        return expr;
    } else if (expect(parser, TOK_INTLITERAL)) {
        token_t int_literal = current(parser);
        advance(parser);

        expression_t* expr = expression_make(EXPR_PRIMARY, location);
        primary_t* primary = primary_make(PRIMARY_INTEGER, location);
        primary->as.integer = strtoll(int_literal.span.data, NULL, 10);
        expr->as.primary = primary;

        return expr;
    } else if (expect(parser, TOK_FLOATLITERAL)) {
        token_t float_literal = current(parser);
        advance(parser);

        expression_t* expr = expression_make(EXPR_PRIMARY, location);
        primary_t* primary = primary_make(PRIMARY_FLOATING, location);
        primary->as.floating = strtod(float_literal.span.data, NULL);
        expr->as.primary = primary;

        return expr;
    } else if (expect(parser, TOK_TRUE)) {
        token_t float_literal = current(parser);
        advance(parser);

        expression_t* expr = expression_make(EXPR_PRIMARY, location);
        primary_t* primary = primary_make(PRIMARY_BOOLEAN, location);
        primary->as.boolean = true;
        expr->as.primary = primary;

        return expr;
    } else if (expect(parser, TOK_FALSE)) {
        token_t float_literal = current(parser);
        advance(parser);

        expression_t* expr = expression_make(EXPR_PRIMARY, location);
        primary_t* primary = primary_make(PRIMARY_BOOLEAN, location);
        primary->as.boolean = false;
        expr->as.primary = primary;

        return expr;
    } else {
        fprintf(stderr, LOCATION_FMT" ERROR: expected expression\n", LOCATION_ARG(location));
        exit(EXIT_FAILURE);
    }
}

expression_t* parse_factor(parser_t* parser) {
    location_t location = current(parser).location;
    expression_t* lhs = parse_primary(parser);

    while (expect(parser, TOK_STAR) || expect(parser, TOK_SLASH)) {
        binary_op_t op = expect(parser, TOK_STAR) ? BINARY_MUL : BINARY_DIV;
        advance(parser);

        expression_t* rhs = parse_primary(parser);

        expression_t* expr = expression_make(EXPR_BINARY, location);
        binary_t* binary = binary_make(op, location, lhs, rhs);
        expr->as.binary = binary;

        lhs = expr;
    }

    return lhs;
}

expression_t* parse_term(parser_t* parser) {
    location_t location = current(parser).location;
    expression_t* lhs = parse_factor(parser);

    while (expect(parser, TOK_PLUS) || expect(parser, TOK_MINUS)) {
        binary_op_t op = expect(parser, TOK_PLUS) ? BINARY_ADD : BINARY_SUB;
        advance(parser);

        expression_t* rhs = parse_factor(parser);

        expression_t* expr = expression_make(EXPR_BINARY, location);
        binary_t* binary = binary_make(op, location, lhs, rhs);
        expr->as.binary = binary;

        lhs = expr;
    }

    return lhs;
}

static expression_t* parse_lower_boolean(parser_t* parser) {
    location_t location = current(parser).location;
    expression_t* lhs = parse_term(parser);

    while (expect(parser, TOK_LESS) || expect(parser, TOK_GREATER) || expect(parser, TOK_EQUAL_EQUAL) || expect(parser, TOK_BANG_EQUAL)) {
        binary_op_t op;
        if (expect(parser, TOK_LESS)) {
            op = BINARY_LESS;
        } else if (expect(parser, TOK_GREATER)) {
            op = BINARY_GREATER;
        } else if (expect(parser, TOK_EQUAL_EQUAL)) {
            op = BINARY_EQUAL;
        } else {
            op = BINARY_NOT_EQUAL;
        }

        advance(parser);

        expression_t* rhs = parse_term(parser);

        expression_t* expr = expression_make(EXPR_BINARY, location);
        binary_t* binary = binary_make(op, location, lhs, rhs);
        expr->as.binary = binary;

        lhs = expr;
    }

    return lhs;
}

static expression_t* parse_higher_boolean(parser_t* parser) {
    location_t location = current(parser).location;
    expression_t* lhs = parse_lower_boolean(parser);

    while (expect(parser, TOK_OR) || expect(parser, TOK_AND)) {
        binary_op_t op = expect(parser, TOK_OR) ? BINARY_OR : BINARY_AND;
        advance(parser);

        expression_t* rhs = parse_lower_boolean(parser);

        expression_t* expr = expression_make(EXPR_BINARY, location);
        binary_t* binary = binary_make(op, location, lhs, rhs);
        expr->as.binary = binary;

        lhs = expr;
    }

    return lhs;
}

expression_t* parse_expression(parser_t* parser) {
    return parse_higher_boolean(parser);
}

block_t* parse_block(parser_t* parser) {
    block_t* block = block_make();

    match(parser, TOK_LCURLY);

    while (!is_eof(parser) && !expect(parser, TOK_RCURLY)) {
        dynarray_push_rval(block->statements, parse_statement(parser));
    }

    match(parser, TOK_RCURLY);

    return block;
}

let_assignment_t* parse_let_assignment(parser_t* parser) {
    location_t location = current(parser).location;

    match(parser, TOK_LET);

    token_t id = current(parser);
    match(parser, TOK_IDENTIFIER);

    match(parser, TOK_EQUAL);

    expression_t* expr = parse_expression(parser);

    match(parser, TOK_SEMICOLON);

    return let_assignment_make(id.span, location, expr);
}

return_t* parse_return(parser_t* parser) {
    location_t location = current(parser).location;
    match(parser, TOK_RETURN);

    if (!expect(parser, TOK_SEMICOLON)) {
        expression_t* expr = parse_expression(parser);

        match(parser, TOK_SEMICOLON);
        return return_make(expr, location);
    }

    match(parser, TOK_SEMICOLON);
    return return_make(NULL, location);
}

statement_t* parse_statement(parser_t* parser) {
    location_t location = current(parser).location;

    if (expect(parser, TOK_LCURLY)) {
        statement_t* statement = statement_make(STMT_BLOCK, location);
        statement->as.block = parse_block(parser);

        return statement;
    } else if (expect(parser, TOK_LET)) {
        statement_t* statement = statement_make(STMT_LET_ASSIGNMENT, location);
        statement->as.let_assignment = parse_let_assignment(parser);

        return statement;
    } else if (expect(parser, TOK_RETURN)) {
        statement_t* statement = statement_make(STMT_RETURN, location);
        statement->as.ret = parse_return(parser);

        return statement;
    } else {
        fprintf(stderr, LOCATION_FMT" ERROR: expected statement\n", LOCATION_ARG(current(parser).location));
        exit(EXIT_FAILURE);
    }
}

parameter_t parse_parameter(parser_t* parser) {
    location_t location = current(parser).location;

    token_t name = current(parser);
    match(parser, TOK_IDENTIFIER);

    match(parser, TOK_COLON);

    token_t type = current(parser);
    if (!expect(parser, TOK_IDENTIFIER)) {
        fprintf(stderr, LOCATION_FMT" ERROR: expected type\n", LOCATION_ARG(current(parser).location));
        exit(EXIT_FAILURE);
    }
    advance(parser);

    return parameter_make(name.span, type.span, location);
}

function_signature_t* parse_function_signature(parser_t* parser) {
    location_t location = current(parser).location;
    match(parser, TOK_DEF);

    token_t name = current(parser);
    match(parser, TOK_IDENTIFIER);

    match(parser, TOK_LPAREN);

    function_signature_t* funsig = function_signature_make(name.span, location);

    bool first = true;
    while (!is_eof(parser) && !expect(parser, TOK_RPAREN)) {
        if (!first) {
            match(parser, TOK_COMMA);
        }

        dynarray_push_rval(funsig->parameters, parse_parameter(parser));
        first = false;
    }

    match(parser, TOK_RPAREN);

    match(parser, TOK_COLON);

    token_t return_type = current(parser);
    if (!expect(parser, TOK_IDENTIFIER)) {
        fprintf(stderr, LOCATION_FMT" ERROR: expected return type\n", LOCATION_ARG(current(parser).location));
        exit(EXIT_FAILURE);
    }
    advance(parser);

    funsig->return_type = return_type.span;

    return funsig;
}

function_definition_t* parse_function_definition(parser_t* parser) {
    location_t location = current(parser).location;
    function_signature_t* funsig = parse_function_signature(parser);
    block_t* body = parse_block(parser);

    return function_definition_make(funsig, body, location);
}
