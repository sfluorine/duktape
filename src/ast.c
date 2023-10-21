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

primary_t* primary_make(primary_kind_t kind, location_t location) {
    primary_t* primary = malloc(sizeof(primary_t));
    primary->kind = kind;
    primary->location = location;

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

binary_t* binary_make(binary_op_t op, location_t location, expression_t* lhs, expression_t* rhs) {
    binary_t* binary = malloc(sizeof(binary_t));
    binary->op = op;
    binary->location = location;
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

block_t* block_make() {
    block_t* block = malloc(sizeof(block_t));
    block->statements = dynarray_create(statement_t*);
    return block;
}

void block_free(block_t* block) {
    for (int i = 0; i < dynarray_length(block->statements); i++) {
        statement_free(block->statements[i]);
    }

    dynarray_destroy(block->statements);
    free(block);
}

let_assignment_t* let_assignment_make(sv_t name, location_t location, expression_t* expr) {
    let_assignment_t* let_assignment = malloc(sizeof(let_assignment_t));
    let_assignment->name = name;
    let_assignment->location = location;
    let_assignment->expr = expr;

    return let_assignment;
}

void let_assignment_free(let_assignment_t* let_assignment) {
    expression_free(let_assignment->expr);
    free(let_assignment);
}

return_t* return_make(expression_t* expr, location_t location) {
    return_t* ret = malloc(sizeof(return_t));
    ret->expr = expr;
    ret->location = location;
    return ret;
}

void return_free(return_t* ret) {
    if (ret->expr) {
        expression_free(ret->expr);
    }

    free(ret);
}

statement_t* statement_make(statement_kind_t kind, location_t location) {
    statement_t* stmt = malloc(sizeof(statement_t));
    stmt->kind = kind;
    stmt->location = location;
    return stmt;
}

void statement_free(statement_t* stmt) {
    switch (stmt->kind) {
        case STMT_BLOCK:
            block_free(stmt->as.block);
            break;
        case STMT_LET_ASSIGNMENT:
            let_assignment_free(stmt->as.let_assignment);
            break;
        case STMT_RETURN:
            return_free(stmt->as.ret);
            break;
    }

    free(stmt);
}
