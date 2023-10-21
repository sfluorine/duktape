#pragma once

#include <ast.h>
#include <stdbool.h>
#include <sv/sv.h>

typedef enum {
    TYPE_KIND_INT,
    TYPE_KIND_FLOAT,
    TYPE_KIND_BOOL,
    TYPE_KIND_VOID,
    TYPE_KIND_COUNT,
} type_kind_t;

typedef struct {
    type_kind_t kind;
    const char* repr;
    bool is_valid_variable_type;
    bool is_valid_return_type;
    bool is_valid_arith_binop_type;
    bool is_valid_bool_binop_type;
    int size;
} type_info_t;

type_info_t get_builtin_type_info(type_kind_t);

typedef struct {
    sv_t name;
    type_info_t type;

    int address;
} compiled_var_t;

compiled_var_t compiled_var_make(sv_t, type_info_t, int);

typedef struct {
    sv_t name;
    type_info_t type;
} compiled_parameter_t;

compiled_parameter_t compiled_parameter_make(sv_t, type_info_t);

typedef struct {
    sv_t name;
    type_info_t return_type;

    compiled_parameter_t* parameters;
} compiled_function_t;

compiled_function_t* compiled_function_make(sv_t, type_info_t);
void compiled_function_free(compiled_function_t*);

typedef enum {
    REG_NONE,
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RDI,
    REG_RSI,
    REG_RBP,
    REG_RSP,
} reg_t;

typedef struct scope_t scope_t;

struct scope_t {
    scope_t* parent;
    compiled_var_t* vars;
};

scope_t* scope_make();
void scope_free(scope_t*);

typedef struct {
    scope_t* scope;
    int frame_size;

    reg_t last_used_reg;
    compiled_function_t** functions;
} compiler_t;

void compiler_init(compiler_t*);
void compiler_deinit(compiler_t*);

void push_scope(compiler_t*);
void pop_scope(compiler_t*);

void insert_var(compiler_t*, compiled_var_t);
compiled_var_t* find_variable(compiler_t*, sv_t);

typedef enum {
    COMP_ERROR_TYPE_MISMATCH,
    COMP_ERROR_TYPE_INVALID_OPERANDS,
    COMP_ERROR_VAR_ALREADY_EXISTS,
    COMP_ERROR_VAR_NOT_EXISTS,
    COMP_ERROR_BLOCK,
    COMP_ERROR_OK,
} compile_error_t;

compile_error_t compile_expression(compiler_t*, type_info_t*, expression_t*);
compile_error_t compile_block(compiler_t*, type_info_t*, block_t*);
compile_error_t compile_let_assignment(compiler_t*, let_assignment_t*);
compile_error_t compile_return(compiler_t*, type_info_t*, return_t*);
compile_error_t compile_statement(compiler_t*, type_info_t*, statement_t*);
