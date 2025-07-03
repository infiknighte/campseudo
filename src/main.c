#include "ast.h"
#include "chunk.h"
#include "parser.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct chunk *interpret(const char *program, const char *source,
                        struct arena **arena, struct vm *vm,
                        struct chunk *chunk) {
  struct scanner scanner;
  struct parser parser;
  scanner_init(&scanner, source);
  parser_init(&parser, &scanner);

  struct ast *ast = parser_parse(&parser, arena, program);

  ast_print(ast);

  chunk_write_ast(&chunk, ast, &vm->objects, &vm->strings);

  vm_interpret(vm, chunk);

  return chunk;
}

static void repl() {
  char line[1024];
  struct vm vm;
  vm_init(&vm);
  struct chunk *chunk = chunk_new();
  struct arena *arena = arena_new(64 * sizeof(struct ast)); // fixed

  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      putchar('\n');
      break;
    }

    chunk = interpret("<stdin>", line, &arena, &vm, chunk);

    vm_reset(&vm);
    chunk_reset(chunk);
    arena_reset(arena);
  }

  vm_free(&vm);
  chunk_free(chunk);
  arena_free(arena);
}

static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = malloc(file_size + 2);
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);

  if (buffer[bytes_read - 1] != '\n') {
    buffer[bytes_read++] = '\n';
  }

  buffer[bytes_read] = 0;

  fclose(file);
  return buffer;
}

void run_file(const char *path) {
  char *source = read_file(path);
  struct vm vm;
  vm_init(&vm);
  struct chunk *chunk = chunk_new();
  struct arena *arena = arena_new(sizeof(struct ast) * 1024);

  chunk = interpret(path, source, &arena, &vm, chunk);

  vm_free(&vm);
  chunk_free(chunk);
  arena_free(arena);
  free(source);
}

int main(int argc, const char *argv[]) {
  // run_file("../example.cpd");
  repl();
  return 0;
}
