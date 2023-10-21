#pragma once

#include <compiler.h>

typedef enum {
    CODEGEN_ERR_TYPECHECK,
    CODEGEN_ERR_OK,
} codegen_error_t;

codegen_error_t codegen_expression(compiler_t*, expression_t*);
