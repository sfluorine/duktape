cmake_minimum_required(VERSION 3.7...3.27)

project(duktape
    DESCRIPTION "VERY VERY TINY TOY COMPILER."
    LANGUAGES C)

add_subdirectory(lib)

add_executable(
    duktape
    src/ast.c
    src/codegen.c
    src/common.c
    src/compiler.c
    src/main.c
    )

target_include_directories(duktape PUBLIC src/)
target_include_directories(duktape PUBLIC lib/)

target_link_libraries(duktape PUBLIC dynarray)
target_link_libraries(duktape PUBLIC sv)
