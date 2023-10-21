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
} type_info_t;

typedef struct {
    sv_t name;
    type_info_t type;

    int address;
} compiled_var_t;

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

typedef struct {
    reg_t last_used_reg;
    compiled_function_t** functions;
} compiler_t;

void compiler_init(compiler_t*);
void compiler_deinit(compiler_t*);

typedef enum {
    TYPE_ERROR_MISMATCH,
    TYPE_ERROR_INVALID_OPERANDS,
    TYPE_ERROR_OK,
} typecheck_error_t;

typecheck_error_t compile_expression(compiler_t*, type_info_t*, expression_t*);
