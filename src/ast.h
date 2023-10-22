#pragma once

#include <common.h>
#include <sv/sv.h>
#include <stdint.h>

typedef struct expression_t expression_t;

typedef struct {
    sv_t name;
    location_t location;
    expression_t** arguments;
} funcall_t;

funcall_t* funcall_make(sv_t, location_t);
void funcall_free(funcall_t*);

typedef enum {
    PRIMARY_INTEGER,
    PRIMARY_FLOATING,
    PRIMARY_IDENTIFIER,
    PRIMARY_BOOLEAN,
    PRIMARY_FUNCALL,
} primary_kind_t;

typedef struct {
    primary_kind_t kind;
    location_t location;

    union {
        int64_t integer;
        double floating;
        sv_t identifier;
        bool boolean;
        funcall_t* funcall;
    } as;
} primary_t;

primary_t* primary_make(primary_kind_t, location_t);
void primary_free(primary_t*);

typedef enum {
    BINARY_ADD,
    BINARY_SUB,
    BINARY_MUL,
    BINARY_DIV,

    BINARY_EQUAL,
    BINARY_NOT_EQUAL,
    BINARY_LESS,
    BINARY_GREATER,
    BINARY_LESS_EQUAL,
    BINARY_GREATER_EQUAL,
    BINARY_OR,
    BINARY_AND,
} binary_op_t;

typedef struct {
    binary_op_t op;
    location_t location;

    expression_t* lhs;
    expression_t* rhs;
} binary_t;

binary_t* binary_make(binary_op_t, location_t, expression_t*, expression_t*);
void binary_free(binary_t*);

typedef enum {
    EXPR_PRIMARY,
    EXPR_BINARY,
} expression_kind_t;

struct expression_t {
    expression_kind_t kind;
    location_t location;

    union {
        primary_t* primary;
        binary_t* binary;
    } as;
};

expression_t* expression_make(expression_kind_t, location_t);
void expression_free(expression_t*);

typedef struct statement_t statement_t;

typedef struct {
    statement_t** statements;
} block_t;

block_t* block_make();
void block_free(block_t*);

typedef struct {
    sv_t name;
    location_t location;
    expression_t* expr;
} let_assignment_t;

let_assignment_t* let_assignment_make(sv_t, location_t, expression_t*);
void let_assignment_free(let_assignment_t*);

typedef struct {
    expression_t* expr;
    location_t location;
} return_t;

return_t* return_make(expression_t*, location_t);
void return_free(return_t*);

typedef enum {
    STMT_BLOCK,
    STMT_LET_ASSIGNMENT,
    STMT_RETURN,
} statement_kind_t;

struct statement_t {
    statement_kind_t kind;
    location_t location;

    union {
        block_t* block;
        let_assignment_t* let_assignment;
        return_t* ret;
    } as;
};

statement_t* statement_make(statement_kind_t, location_t);
void statement_free(statement_t*);

typedef struct {
    sv_t name;
    sv_t type;
    location_t location;
} parameter_t;

parameter_t parameter_make(sv_t, sv_t, location_t);

typedef struct {
    location_t location;
    sv_t name;
    parameter_t* parameters;
    sv_t return_type;
} function_signature_t;

function_signature_t* function_signature_make(sv_t, location_t);
void function_signature_free(function_signature_t*);

typedef struct {
    function_signature_t* funsig;
    block_t* body;
    location_t location;
} function_definition_t;

function_definition_t* function_definition_make(function_signature_t*, block_t*, location_t);
void function_definition_free(function_definition_t*);
