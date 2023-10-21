#pragma once

#include <compiler.h>

void codegen_expression(compiler_t*, expression_t*);
void codegen_let_assignment(compiler_t*, let_assignment_t*);
