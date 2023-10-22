#include <assert.h>
#include <compiler.h>
#include <dynarray/dynarray.h>
#include <stdio.h>
#include <stdlib.h>

static type_info_t type_infos[TYPE_KIND_COUNT] = {
    [TYPE_KIND_INT]   = { .kind = TYPE_KIND_INT,   .repr = "int", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = true,  .is_valid_bool_binop_type = true,   .size = 8, .is_valid_lg_gt_value_type = true  },
    [TYPE_KIND_FLOAT] = { .kind = TYPE_KIND_FLOAT, .repr = "float", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = true,  .is_valid_bool_binop_type = true, .size = 8, .is_valid_lg_gt_value_type = true  },
    [TYPE_KIND_BOOL]  = { .kind = TYPE_KIND_BOOL,  .repr = "bool", .is_valid_variable_type = true,  .is_valid_return_type = true, .is_valid_arith_binop_type = false, .is_valid_bool_binop_type = true,  .size = 1, .is_valid_lg_gt_value_type = false },
    [TYPE_KIND_VOID]  = { .kind = TYPE_KIND_VOID,  .repr = "void", .is_valid_variable_type = false, .is_valid_return_type = true, .is_valid_arith_binop_type = false, .is_valid_bool_binop_type = false, .size = 0, .is_valid_lg_gt_value_type = false },
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

static bool check_op_is_lg_gt(binary_op_t op) {
    switch (op) {
        case BINARY_LESS:
        case BINARY_GREATER:
        case BINARY_LESS_EQUAL:
        case BINARY_GREATER_EQUAL:
            return true;
        default:
            return false;
    }
}

static compile_error_t check_valid_binop(binary_op_t op, type_info_t lhs, type_info_t rhs) {
    if (lhs.kind != rhs.kind) {
        return COMP_ERROR_TYPE_MISMATCH;
    }

    if (check_op_is_bool(op) && !lhs.is_valid_bool_binop_type) {
        return COMP_ERROR_TYPE_INVALID_OPERANDS;
    }

    if (!check_op_is_bool(op) && !lhs.is_valid_arith_binop_type) {
        return COMP_ERROR_TYPE_INVALID_OPERANDS;
    }

    if (check_op_is_lg_gt(op) && !lhs.is_valid_lg_gt_value_type) {
        return COMP_ERROR_TYPE_INVALID_OPERANDS;
    }

    return COMP_ERROR_OK;
}

type_info_t get_builtin_type_info(type_kind_t kind) {
    return type_infos[kind];
}

compiled_var_t compiled_var_make(sv_t name, type_info_t type, int address) {
    return (compiled_var_t) {
        .name = name,
        .type = type,
        .address = address,
    };
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
    compiler->scope = NULL;
    compiler->frame_size = 0;

    compiler->last_used_reg = REG_NONE;
    compiler->functions = dynarray_create(compiled_function_t*);
}

void compiler_deinit(compiler_t* compiler) {
    dynarray_destroy(compiler->functions);
}

scope_t* scope_make() {
    scope_t* scope = malloc(sizeof(scope_t));
    scope->parent = NULL;
    scope->vars = dynarray_create(compiled_var_t);

    return scope;
}

void scope_free(scope_t* scope) {
    dynarray_destroy(scope->vars);
    free(scope);
}

void push_scope(compiler_t* compiler)  {
    scope_t* child = scope_make();
    child->parent = compiler->scope;
    compiler->scope = child;
}

void pop_scope(compiler_t* compiler) {
    scope_t* current = compiler->scope;
    compiler->scope = current->parent;

    int dealloc_size = 0;
    for (int i = 0; i < dynarray_length(current->vars); i++) {
        dealloc_size += current->vars[i].type.size;
    }

    compiler->frame_size -= dealloc_size;
    scope_free(current);
}

void insert_var(compiler_t* compiler, compiled_var_t compiled_var) {
    scope_t* current = compiler->scope;
    dynarray_push(current->vars, compiled_var);

    compiler->frame_size += compiled_var.type.size;
}

compiled_var_t* find_variable(compiler_t* compiler, sv_t name) {
    compiled_var_t* var = NULL;

    for (scope_t* scope = compiler->scope; scope != NULL; scope = scope->parent) {
        for (int i = 0; i < dynarray_length(scope->vars); i++) {
            if (sv_equals(scope->vars[i].name, name)) {
                var = &scope->vars[i];
            }
        }
    }

    return var;
}

static compile_error_t resolve_variable(compiler_t* compiler, type_info_t* type_info, primary_t* primary) {
    compiled_var_t* var = find_variable(compiler, primary->as.identifier);
    if (!var) {
        fprintf(stderr, LOCATION_FMT" ERROR: referenced variable '"SV_FMT"' does not exists\n", LOCATION_ARG(primary->location), SV_ARG(primary->as.identifier));
        return COMP_ERROR_VAR_NOT_EXISTS;
    }

    *type_info = var->type;
    return COMP_ERROR_OK;
}

static bool is_binop_result_bool(binary_op_t op) {
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

compile_error_t compile_expression(compiler_t* compiler, type_info_t* type_info, expression_t* expr) {
    if (expr->kind == EXPR_PRIMARY) {
        primary_t* primary = expr->as.primary;

        switch (primary->kind) {
            case PRIMARY_INTEGER:
                *type_info = type_infos[TYPE_KIND_INT];
                return COMP_ERROR_OK;
            case PRIMARY_FLOATING:
                *type_info = type_infos[TYPE_KIND_FLOAT];
                return COMP_ERROR_OK;
            case PRIMARY_IDENTIFIER:
                return resolve_variable(compiler, type_info, primary);
            case PRIMARY_BOOLEAN:
                *type_info = type_infos[TYPE_KIND_BOOL];
                return COMP_ERROR_OK;
            case PRIMARY_FUNCALL:
                assert(false && "unimplemented");
        }
    } else {
        binary_t* binary = expr->as.binary;

        type_info_t lhs;
        compile_error_t err = compile_expression(compiler, &lhs, binary->lhs);
        if (err != COMP_ERROR_OK) {
            return err;
        }

        type_info_t rhs;
        err = compile_expression(compiler, &rhs, binary->rhs);
        if (err != COMP_ERROR_OK) {
            return err;
        }

        compile_error_t error = check_valid_binop(binary->op, lhs, rhs);

        switch (error) {
            case COMP_ERROR_TYPE_MISMATCH:
                fprintf(stderr, LOCATION_FMT" ERROR: binary expr type mismatch:\n  lhs -> %s\n  rhs -> %s\n", LOCATION_ARG(binary->location), lhs.repr, rhs.repr);
                return error;
            case COMP_ERROR_TYPE_INVALID_OPERANDS:
                fprintf(stderr, LOCATION_FMT" ERROR: binary expr unsupported operands:\n  lhs -> %s\n  rhs -> %s\n", LOCATION_ARG(binary->location), lhs.repr, rhs.repr);
                return error;
            default:
                if (is_binop_result_bool(binary->op)) {
                    *type_info = get_builtin_type_info(TYPE_KIND_BOOL);
                } else {
                    *type_info = lhs;
                }
                return error;
        }
    }
}

compile_error_t compile_block(compiler_t* compiler, type_info_t* type_info, block_t* block) {
    compile_error_t error = COMP_ERROR_OK;

    for (int i = 0; i < dynarray_length(block->statements); i++) {
        if (!compile_statement(compiler, type_info, block->statements[i])) {
            error = COMP_ERROR_BLOCK;
        }
    }

    return error;
}

compile_error_t compile_let_assignment(compiler_t* compiler, let_assignment_t* let_assignment) {
    type_info_t expr_type = {0};
    compile_error_t error = compile_expression(compiler, &expr_type, let_assignment->expr);

    if (error != COMP_ERROR_OK) {
        return error;
    }

    if (find_variable(compiler, let_assignment->name)) {
        fprintf(stderr, 
                LOCATION_FMT" ERROR: cannot declare '"SV_FMT"' since it's already exists\n",
                LOCATION_ARG(let_assignment->location),
                SV_ARG(let_assignment->name));
        return COMP_ERROR_VAR_ALREADY_EXISTS;
    }

    compiled_var_t compiled_var = compiled_var_make(let_assignment->name, expr_type, compiler->frame_size);
    insert_var(compiler, compiled_var);

    return COMP_ERROR_OK;
}

compile_error_t compile_return(compiler_t* compiler, type_info_t* type_info, return_t* ret) {
    if (ret->expr) {
        return compile_expression(compiler, type_info, ret->expr);
    }

    return COMP_ERROR_OK;
}

compile_error_t compile_statement(compiler_t* compiler, type_info_t* type_info, statement_t* stmt) {
    switch (stmt->kind) {
        case STMT_BLOCK:
            return compile_block(compiler, type_info, stmt->as.block);
        case STMT_LET_ASSIGNMENT:
            return compile_let_assignment(compiler, stmt->as.let_assignment);
        case STMT_RETURN:
            return compile_return(compiler, type_info, stmt->as.ret);
    }
}
