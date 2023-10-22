#pragma once

#include <ast.h>
#include <token.h>

typedef struct {
    token_t* tokens;
    int cursor;
} parser_t;

void parser_init(parser_t*, token_t*);
void parser_deinit(parser_t*);

expression_t* parse_primary(parser_t*);
expression_t* parse_factor(parser_t*);
expression_t* parse_term(parser_t*);
expression_t* parse_expression(parser_t*);

block_t* parse_block(parser_t*);
let_assignment_t* parse_let_assignment(parser_t*);
return_t* parse_return(parser_t*);
statement_t* parse_statement(parser_t*);

parameter_t parse_parameter(parser_t*);
function_signature_t* parse_function_signature(parser_t*);
function_definition_t* parse_function_definition(parser_t*);
