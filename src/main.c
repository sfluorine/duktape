#include <compiler.h>
#include <codegen.h>

int main() {
    compiler_t compiler;
    compiler_init(&compiler);

    block_t* block = block_make();

    type_info_t type = get_builtin_type_info(TYPE_KIND_VOID);
    compile_block(&compiler, &type, block);
    codegen_block(&compiler, block);

    block_free(block);

    compiler_deinit(&compiler);
}
