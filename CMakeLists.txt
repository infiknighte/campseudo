cmake_minimum_required (VERSION 3.14)
project (campseudo C)
set(C_STANDARD 23)

add_executable (campseudo
    src/main.c include/common.h
    src/scanner.c include/scanner.h
    src/chunk.c include/chunk.h
    src/value.c include/value.h
    src/vm.c include/vm.h
    src/stack.c include/stack.h
    src/ast.c include/ast.h
    src/parser.c include/parser.h
    src/obj.c include/obj.h
    src/memory.c include/memory.h
)
target_include_directories (campseudo PRIVATE include)
