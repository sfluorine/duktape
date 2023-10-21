#include <ast.h>
#include <dynarray/dynarray.h>
#include <stdlib.h>

funcall_t* funcall_make(sv_t name) {
    funcall_t* funcall = malloc(sizeof(funcall_t));
    funcall->name = name;
    funcall->arguments = dynarray_create(expression_t*);

    return funcall;
}

void funcall_free(funcall_t* funcall) {
    dynarray_destroy(funcall->arguments);
    free(funcall);
}

primary_t* primary_make(primary_kind_t kind) {
    primary_t* primary = malloc(sizeof(primary_t));
    primary->kind = kind;

    return primary;
}

void primary_free(primary_t* primary) {
    switch (primary->kind) {
        case PRIMARY_INTEGER:
            break;
        case PRIMARY_FLOATING:
            break;
        case PRIMARY_IDENTIFIER:
            break;
        case PRIMARY_FUNCALL:
            funcall_free(primary->as.funcall);
            break;
    }

    free(primary);
}

binary_t* binary_make(binary_op_t op, expression_t* lhs, expression_t* rhs) {
    binary_t* binary = malloc(sizeof(binary_t));
    binary->op = op;
    binary->lhs = lhs;
    binary->rhs = rhs;

    return binary;
}

void binary_free(binary_t* binary) {
    expression_free(binary->lhs);
    expression_free(binary->rhs);
    free(binary);
}

expression_t* expression_make(expression_kind_t kind, location_t location) {
    expression_t* expr = malloc(sizeof(expression_t));
    expr->kind = kind;
    expr->location = location;

    return expr;
}

void expression_free(expression_t* expr) {
    switch (expr->kind) {
        case EXPR_PRIMARY:
            primary_free(expr->as.primary);
            break;
        case EXPR_BINARY:
            binary_free(expr->as.binary);
            break;
    }

    free(expr);
}
