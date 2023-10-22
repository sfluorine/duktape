#include <assert.h>
#include <compiler.h>
#include <dynarray/dynarray.h>
#include <stdio.h>
#include <stdlib.h>

static type_info_t builtin_type_infos[TYPE_KIND_COUNT] = {
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
    for (int i = 0; i < dynarray_length(compiler->functions); i++) {
        compiled_function_free(compiler->functions[i]);
    }

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

void insert_fun(compiler_t* compiler, compiled_function_t* fun) {
    dynarray_push(compiler->functions, fun);
}

compiled_function_t* find_function(compiler_t* compiler, sv_t name) {
    compiled_function_t* fun = NULL;

    for (int i = 0; i < dynarray_length(compiler->functions); i++) {
        if (sv_equals(compiler->functions[i]->name, name)) {
            fun = compiler->functions[i];
        }
    }

    return fun;
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

compile_error_t compile_funcall(compiler_t* compiler, type_info_t* type_info, funcall_t* funcall) {
    compiled_function_t* fun = find_function(compiler, funcall->name);
    if (!fun) {
        fprintf(stderr,
                LOCATION_FMT" ERROR: no such function '"SV_FMT"'\n",
                LOCATION_ARG(funcall->location),
                SV_ARG(funcall->name));

        return COMP_ERROR_FUN_NOT_EXISTS;
    }

    if (dynarray_length(fun->parameters) != dynarray_length(funcall->arguments)) {
        fprintf(stderr,
                LOCATION_FMT" ERROR: '"SV_FMT"' expected %zu arguments, but got %zu\n",
                LOCATION_ARG(funcall->location),
                SV_ARG(funcall->name), dynarray_length(fun->parameters), dynarray_length(funcall->arguments));

        return COMP_ERROR_FUN_ARITY_NOT_MATCH;
    }

    for (int i = 0; i < dynarray_length(funcall->arguments); i++) {
        type_info_t expr_type = {0};
        compile_expression(compiler, &expr_type, funcall->arguments[i]);

        if (fun->parameters[i].type.kind != expr_type.kind) {
            fprintf(stderr,
                    LOCATION_FMT" ERROR: '"SV_FMT"' parameter type for function '"SV_FMT"' does not match. expected '%s', but got '%s'\n",
                    LOCATION_ARG(funcall->arguments[i]->location),
                    SV_ARG(fun->parameters[i].name),
                    SV_ARG(fun->name),
                    fun->parameters[i].type.repr,
                    expr_type.repr);

            return COMP_ERROR_TYPE_MISMATCH;
        }
    }

    *type_info = fun->return_type;

    return COMP_ERROR_OK;
}

compile_error_t compile_expression(compiler_t* compiler, type_info_t* type_info, expression_t* expr) {
    if (expr->kind == EXPR_PRIMARY) {
        primary_t* primary = expr->as.primary;

        switch (primary->kind) {
            case PRIMARY_INTEGER:
                *type_info = builtin_type_infos[TYPE_KIND_INT];
                return COMP_ERROR_OK;
            case PRIMARY_FLOATING:
                *type_info = builtin_type_infos[TYPE_KIND_FLOAT];
                return COMP_ERROR_OK;
            case PRIMARY_IDENTIFIER:
                return resolve_variable(compiler, type_info, primary);
            case PRIMARY_BOOLEAN:
                *type_info = builtin_type_infos[TYPE_KIND_BOOL];
                return COMP_ERROR_OK;
            case PRIMARY_FUNCALL:
                return compile_funcall(compiler, type_info, primary->as.funcall);
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
                    *type_info = builtin_type_infos[TYPE_KIND_BOOL];
                } else {
                    *type_info = lhs;
                }
                return error;
        }
    }
}

compile_error_t compile_block(compiler_t* compiler, type_info_t* type_info, block_t* block) {
    for (int i = 0; i < dynarray_length(block->statements); i++) {
        compile_error_t error = compile_statement(compiler, type_info, block->statements[i]);
        if (error != COMP_ERROR_OK) {
            return error;
        }
    }

    return COMP_ERROR_OK;
}

compile_error_t compile_let_assignment(compiler_t* compiler, let_assignment_t* let_assignment) {
    type_info_t expr_type = {0};
    compile_error_t error = compile_expression(compiler, &expr_type, let_assignment->expr);

    if (error != COMP_ERROR_OK) {
        return error;
    }

    if (find_variable(compiler, let_assignment->name)) {
        fprintf(stderr, 
                LOCATION_FMT" ERROR: cannot declare variable '"SV_FMT"' since it's already exists\n",
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

static type_info_t* resolve_type(compiler_t* compiler, sv_t type) {
    if (sv_equals(type, sv_make_from("int"))) {
        return &builtin_type_infos[TYPE_KIND_INT];
    } else if (sv_equals(type, sv_make_from("float"))) {
        return &builtin_type_infos[TYPE_KIND_FLOAT];
    }  else if (sv_equals(type, sv_make_from("bool"))) {
        return &builtin_type_infos[TYPE_KIND_BOOL];
    } else if (sv_equals(type, sv_make_from("void"))) {
        return &builtin_type_infos[TYPE_KIND_VOID];
    } else {
        return NULL;
    }
}

compile_error_t compile_parameter(compiler_t* compiler, compiled_parameter_t* compiled_parameter, parameter_t parameter) {
    type_info_t* type = resolve_type(compiler, parameter.type);
    if (!type) {
        fprintf(stderr, LOCATION_FMT" ERROR: no such type '"SV_FMT"'\n", LOCATION_ARG(parameter.location), SV_ARG(parameter.type));
        return COMP_ERROR_TYPE_NOT_EXISTS;
    }

    if (!type->is_valid_variable_type) {
        fprintf(stderr, LOCATION_FMT" ERROR: cannot make a parameter out of '"SV_FMT"'\n", LOCATION_ARG(parameter.location), SV_ARG(parameter.type));
        return COMP_ERROR_UNEXPECTED_TYPE;
    }

    *compiled_parameter = compiled_parameter_make(parameter.name, *type);

    return COMP_ERROR_OK;
}

compile_error_t compile_function_signature(compiler_t* compiler, type_info_t* type_info, compiled_parameter_t** parameters, function_signature_t* funsig) {
    if (find_function(compiler, funsig->name)) {
        fprintf(stderr,
                LOCATION_FMT" ERROR: cannot declare function '"SV_FMT"' since it's already exists\n",
                LOCATION_ARG(funsig->location),
                SV_ARG(funsig->name));

        return COMP_ERROR_FUN_ALREADY_EXISTS;
    }

    type_info_t* type = resolve_type(compiler, funsig->return_type);
    if (!type) {
        fprintf(stderr, LOCATION_FMT" ERROR: no such type '"SV_FMT"'\n", LOCATION_ARG(funsig->location), SV_ARG(funsig->return_type));
        return COMP_ERROR_TYPE_NOT_EXISTS;
    }

    compiled_parameter_t* params = dynarray_create(compiled_parameter_t);
    for (int i = 0; i < dynarray_length(funsig->parameters); i++) {
        compiled_parameter_t param = {0};
        compile_error_t error = compile_parameter(compiler, &param, funsig->parameters[i]);

        if (error != COMP_ERROR_OK) {
            return error;
        }

        for (int i = 0; i < dynarray_length(params); i++) {
            if (sv_equals(params[i].name, param.name)) {
                fprintf(stderr,
                        LOCATION_FMT" ERROR: cannot declare parameter '"SV_FMT"' since it's already exists\n",
                        LOCATION_ARG(funsig->parameters[i].location),
                        SV_ARG(param.name));
                return COMP_ERROR_VAR_ALREADY_EXISTS;
            }
        }

        dynarray_push(params, param);
    }

    *type_info = *type;
    *parameters = params;

    return COMP_ERROR_OK;
}

compile_error_t compile_function_definition(compiler_t* compiler, function_definition_t* fundef) {
    type_info_t funsig_type = {0};
    compiled_parameter_t* params = NULL;

    compile_error_t error = compile_function_signature(compiler, &funsig_type, &params, fundef->funsig);
    if (error != COMP_ERROR_OK) {
        return error;
    }

    for (int i = 0; i < dynarray_length(params); i++) {
        compiled_var_t var = compiled_var_make(params[i].name, params[i].type, compiler->frame_size);
        insert_var(compiler, var);
    }

    type_info_t return_type = builtin_type_infos[TYPE_KIND_VOID];
    error = compile_block(compiler, &return_type, fundef->body);
    if (error != COMP_ERROR_OK) {
        return error;
    }

    if (funsig_type.kind != return_type.kind) {
        fprintf(stderr, LOCATION_FMT" ERROR: unexpected return type. expected '%s', but got '%s'\n", LOCATION_ARG(fundef->location), funsig_type.repr, return_type.repr);
        return COMP_ERROR_UNEXPECTED_TYPE;
    }

    compiled_function_t* fun = compiled_function_make(fundef->funsig->name, return_type);
    fun->parameters = params;
    insert_fun(compiler, fun);

    return COMP_ERROR_OK;
}
