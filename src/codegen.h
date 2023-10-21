#pragma once

#include <compiler.h>

void codegen_expression(compiler_t*, expression_t*);
void codegen_block(compiler_t*, block_t*);
void codegen_let_assignment(compiler_t*, let_assignment_t*);
void codegen_return(compiler_t*, return_t*);
void codegen_statement(compiler_t*, statement_t*);
