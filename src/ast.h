#pragma once

#include "common.h"
#include <sv/sv.h>
#include <stdint.h>

typedef struct expression_t expression_t;

typedef struct {
    sv_t name;
    expression_t** arguments;
} funcall_t;

funcall_t* funcall_make(sv_t);
void funcall_free(funcall_t*);

typedef enum {
    PRIMARY_INTEGER,
    PRIMARY_FLOATING,
    PRIMARY_IDENTIFIER,
    PRIMARY_FUNCALL,
} primary_kind_t;

typedef struct {
    primary_kind_t kind;
    union {
        int64_t integer;
        double floating;
        sv_t identifier;
        funcall_t* funcall;
    } as;
} primary_t;

primary_t* primary_make(primary_kind_t);
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
    expression_t* lhs;
    expression_t* rhs;
} binary_t;

binary_t* binary_make(binary_op_t, expression_t*, expression_t*);
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
