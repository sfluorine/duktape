#include <stdio.h>
#include <compiler.h>
#include <codegen.h>

int main() {
    compiler_t compiler;
    compiler_init(&compiler);

    expression_t* lhs = expression_make(EXPR_PRIMARY, location_make(1, 1));
    primary_t* lhs_primary = primary_make(PRIMARY_INTEGER);
    lhs_primary->as.integer = 5;
    lhs->as.primary = lhs_primary;

    expression_t* rhs = expression_make(EXPR_PRIMARY, location_make(1, 5));
    primary_t* rhs_primary = primary_make(PRIMARY_INTEGER);
    rhs_primary->as.integer = 5;
    rhs->as.primary = rhs_primary;

    expression_t* lhs_root = expression_make(EXPR_BINARY, location_make(1, 1));
    binary_t* lhs_root_binary = binary_make(BINARY_ADD, lhs, rhs);
    lhs_root->as.binary = lhs_root_binary;

    expression_t* rhs_root = expression_make(EXPR_PRIMARY, location_make(1, 1));
    primary_t* rhs_root_prim = primary_make(PRIMARY_INTEGER);
    rhs_root_prim->as.integer = 2;
    rhs_root->as.primary = rhs_root_prim;

    expression_t* root = expression_make(EXPR_BINARY, location_make(2, 6));
    binary_t* root_binary = binary_make(BINARY_DIV, lhs_root, rhs_root);
    root_binary->lhs = lhs_root;
    root_binary->rhs = rhs_root;

    root->as.binary = root_binary;

    if (codegen_expression(&compiler, root) != CODEGEN_ERR_OK) {
        fprintf(stderr, "ERROR: codegen failed\n");
    }

    expression_free(root);

    compiler_deinit(&compiler);
    return 0;
}