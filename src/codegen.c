#include <assert.h>
#include <codegen.h>
#include <stdio.h>

static bool is_primary(expression_t* expr) {
    return expr->kind == EXPR_PRIMARY;
}

static const char* reg_to_str(reg_t reg) {
    switch (reg) {
        case REG_RAX:
            return "rax";
        case REG_RBX:
            return "rbx";
        case REG_RCX:
            return "rcx";
        case REG_RDX:
            return "rdx";
        case REG_RDI:
            return "rdi";
        case REG_RSI:
            return "rsi";
        case REG_RBP:
            return "rbp";
        case REG_RSP:
            return "rsp";
        default:
            return "";
    }
}

static void mov_reg_to_reg(compiler_t* compiler, reg_t dst, reg_t src) {
    printf("mov %s, %s\n", reg_to_str(dst), reg_to_str(src));
}

static void mov_constant_to_reg(compiler_t* compiler, reg_t dst, int64_t src) {
    printf("mov %s, %ld\n", reg_to_str(dst), src);
    compiler->last_used_reg = dst;
}

static void codegen_primary(compiler_t* compiler, primary_t* primary) {
    if (primary->kind == PRIMARY_INTEGER) {
        mov_constant_to_reg(compiler, compiler->last_used_reg + 1, primary->as.integer);
    } else {
        assert(false && "unimplemented");
    }
}

static void codegen_binop(compiler_t* compiler, binary_op_t op) {
    if (op == BINARY_ADD) {
        printf("add %s, %s\n", reg_to_str(compiler->last_used_reg - 1), reg_to_str(compiler->last_used_reg));
        compiler->last_used_reg = compiler->last_used_reg - 1;
    } else if (op == BINARY_SUB) {
        printf("sub %s, %s\n", reg_to_str(compiler->last_used_reg - 1), reg_to_str(compiler->last_used_reg));
        compiler->last_used_reg = compiler->last_used_reg - 1;
    } else if (op == BINARY_MUL) {
        printf("imul %s, %s\n", reg_to_str(compiler->last_used_reg - 1), reg_to_str(compiler->last_used_reg));
        compiler->last_used_reg = compiler->last_used_reg - 1;
    } else if (op == BINARY_DIV) {
        if (compiler->last_used_reg > 2) {
            printf("push rax\n");
            mov_reg_to_reg(compiler, REG_RAX, compiler->last_used_reg - 1);
            printf("div %s\n", reg_to_str(compiler->last_used_reg));
            mov_reg_to_reg(compiler, compiler->last_used_reg - 1, REG_RAX);
            printf("pop rax\n");
        } else {
            printf("div %s\n", reg_to_str(compiler->last_used_reg));
        }

        compiler->last_used_reg = compiler->last_used_reg - 1;
    } else {
        assert(false && "unimplemented");
    }
}

void codegen_expression(compiler_t* compiler, expression_t* expr) {
    if (expr->kind == EXPR_PRIMARY) {
        primary_t* primary = expr->as.primary;
        codegen_primary(compiler, primary);
    } else  {
        binary_t* binary = expr->as.binary;
        if (is_primary(binary->lhs) && !is_primary(binary->rhs)) {
            codegen_expression(compiler, binary->rhs);
            codegen_expression(compiler, binary->lhs);
            printf("xchg %s, %s\n", reg_to_str(compiler->last_used_reg - 1), reg_to_str(compiler->last_used_reg));
        } else {
            codegen_expression(compiler, binary->lhs);
            codegen_expression(compiler, binary->rhs);
        }

        codegen_binop(compiler, binary->op);
    }
}

void codegen_let_assignment(compiler_t* compiler, let_assignment_t* let_assignment) {
    codegen_expression(compiler, let_assignment->expr);

    compiled_var_t* var = find_variable(compiler, let_assignment->name);

    printf("mov [rbp - %d], rax\n", var->address + var->type.size);
}
