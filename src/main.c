#include <compiler.h>
#include <codegen.h>
#include <dynarray/dynarray.h>
#include <errno.h>
#include <lexer.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* slurp_file(const char* filepath) {
    FILE* stream = fopen(filepath, "r");
    if (!stream) {
        fprintf(stderr, "ERROR: cannot open file '%s': %s", filepath, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(stream, 0, SEEK_END);
    long size = ftell(stream);
    fseek(stream, 0, SEEK_SET);

    if (size == 0) {
        fclose(stream);
        return NULL;
    }

    char* buffer = malloc(sizeof(char) * size + 1);
    fread(buffer, sizeof(char), size, stream);
    buffer[size] = 0;

    fclose(stream);

    return buffer;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    char* buffer = slurp_file(argv[1]);

    lexer_t lexer;
    lexer_init(&lexer, buffer);

    token_t* tokens = get_tokens(&lexer);
    parser_t parser;
    parser_init(&parser, tokens);

    compiler_t compiler;
    compiler_init(&compiler);

    push_scope(&compiler);

    function_definition_t* add = parse_function_definition(&parser);
    compile_function_definition(&compiler, add);
    function_definition_free(add);

    pop_scope(&compiler);

    push_scope(&compiler);

    function_definition_t* main = parse_function_definition(&parser);
    compile_function_definition(&compiler, main);
    function_definition_free(main);

    pop_scope(&compiler);

    compiler_deinit(&compiler);
    parser_deinit(&parser);
    lexer_deinit(&lexer);
}
