#include <assert.h>
#include <codegen.h>
#include <dynarray/dynarray.h>
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
    } else if (primary->kind == PRIMARY_IDENTIFIER) {
        compiled_var_t* var = find_variable(compiler, primary->as.identifier);

        // TODO: no not hardcode the addressing size
        printf("mov %s, qword [rbp - %d]\n", reg_to_str(compiler->last_used_reg), var->address + var->type.size);
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

void codegen_block(compiler_t* compiler, block_t* block) {
    for (int i = 0; i < dynarray_length(block->statements); i++) {
        codegen_statement(compiler, block->statements[i]);
    }
}

void codegen_let_assignment(compiler_t* compiler, let_assignment_t* let_assignment) {
    codegen_expression(compiler, let_assignment->expr);

    compiled_var_t* var = find_variable(compiler, let_assignment->name);

    printf("mov [rbp - %d], rax\n", var->address + var->type.size);
}

void codegen_return(compiler_t* compiler, return_t* ret) {
    if (ret->expr) {
        codegen_expression(compiler, ret->expr);
    }

    printf("mov rsp, rbp\n");
    printf("pop rbp\n");
    printf("ret\n");
}

void codegen_statement(compiler_t* compiler, statement_t* stmt) {
    switch (stmt->kind) {
        case STMT_BLOCK:
            codegen_block(compiler, stmt->as.block);
            break;
        case STMT_LET_ASSIGNMENT:
            codegen_let_assignment(compiler, stmt->as.let_assignment);
            break;
        case STMT_RETURN:
            codegen_return(compiler, stmt->as.ret);
            break;
    }
}
