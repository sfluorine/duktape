#include <assert.h>
#include <compiler.h>
#include <dynarray/dynarray.h>
#include <stdio.h>
#include <stdlib.h>

static type_info_t type_infos[TYPE_KIND_COUNT] = {
    [TYPE_KIND_INT]   = { .kind = TYPE_KIND_INT,   .repr = "int", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = true,  .is_valid_bool_binop_type = true },
    [TYPE_KIND_FLOAT] = { .kind = TYPE_KIND_FLOAT, .repr = "float", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = true,  .is_valid_bool_binop_type = true },
    [TYPE_KIND_BOOL]  = { .kind = TYPE_KIND_BOOL,  .repr = "bool", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = false, .is_valid_bool_binop_type = true  },
    [TYPE_KIND_VOID]  = { .kind = TYPE_KIND_VOID,  .repr = "void", .is_valid_variable_type = false, .is_valid_return_type = true, .is_valid_arith_binop_type = false, .is_valid_bool_binop_type = false },
};

static bool check_op_is_bool(binary_op_t op) {
    switch (op) {
        case BINARY_EQUAL:
        case BINARY_NOT_EQUAL:
        case BINARY_LESS:
        case BINARY_GREATER:
        case BINARY_LESS_EQUAL:
        case BINARY_GREATER_EQUAL:
        case BINARY_OR:
        case BINARY_AND:
            return true;
        default:
            return false;
    }
}

static typecheck_error_t check_valid_binop(binary_op_t op, type_info_t lhs, type_info_t rhs) {
    if (lhs.kind != rhs.kind) {
        return TYPE_ERROR_MISMATCH;
    }

    if (check_op_is_bool(op) && !lhs.is_valid_bool_binop_type) {
        return TYPE_ERROR_INVALID_OPERANDS;
    }

    if (!check_op_is_bool(op) && !lhs.is_valid_arith_binop_type) {
        return TYPE_ERROR_INVALID_OPERANDS;
    }

    return TYPE_ERROR_OK;
}

compiled_parameter_t compiled_parameter_make(sv_t name, type_info_t type) {
    return (compiled_parameter_t) {
        .name = name,
        .type = type,
    };
}

compiled_function_t* compiled_function_make(sv_t name, type_info_t return_type) {
    compiled_function_t* function = malloc(sizeof(compiled_function_t));
    function->name = name;
    function->return_type = return_type;

    // allocate parameters with dynamic array.
    function->parameters = dynarray_create(compiled_parameter_t);

    return function;
}

void compiled_function_free(compiled_function_t* function) {
    dynarray_destroy(function->parameters);
    free(function);
}

void compiler_init(compiler_t* compiler) {
    compiler->last_used_reg = REG_NONE;
    compiler->functions = dynarray_create(compiled_function_t*);
}

void compiler_deinit(compiler_t* compiler) {
    dynarray_destroy(compiler->functions);
}

typecheck_error_t compile_expression(compiler_t* compiler, type_info_t* type_info, expression_t* expr) {
    if (expr->kind == EXPR_PRIMARY) {
        primary_t* primary = expr->as.primary;

        switch (primary->kind) {
            case PRIMARY_INTEGER:
                *type_info = type_infos[TYPE_KIND_INT];
                return TYPE_ERROR_OK;
            case PRIMARY_FLOATING:
                *type_info = type_infos[TYPE_KIND_FLOAT];
                return TYPE_ERROR_OK;
            case PRIMARY_IDENTIFIER:
                assert(false && "unimplemented");
            case PRIMARY_FUNCALL:
                assert(false && "unimplemented");
        }
    } else {
        binary_t* binary = expr->as.binary;

        type_info_t lhs;
        compile_expression(compiler, &lhs, binary->lhs);

        type_info_t rhs;
        compile_expression(compiler, &rhs, binary->rhs);

        typecheck_error_t error = check_valid_binop(binary->op, lhs, rhs);

        switch (error) {
            case TYPE_ERROR_MISMATCH:
                fprintf(stderr, LOCATION_FMT" ERROR: binary expr type mismatch:\n  lhs -> %s\n  rhs -> %s\n", LOCATION_ARG(expr->location), lhs.repr, rhs.repr);
                return error;
            case TYPE_ERROR_INVALID_OPERANDS:
                fprintf(stderr, LOCATION_FMT" ERROR: binary expr invalid operands:\n  lhs -> %s\n  rhs -> %s\n", LOCATION_ARG(expr->location), lhs.repr, rhs.repr);
                return error;
            default:
                *type_info = lhs;
                return error;
        }
    }
}
