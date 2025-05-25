#include "ast.h"
#include "chunk.h"
#include "parser.h"
#include "scanner.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void interpret(const char *source) {
  struct scanner scanner;
  scanner_init(&scanner, source);
  uint32_t line = 0;
  for (;;) {
    struct token token = scanner_scan_token(&scanner);
    if (token.line != line) {
      printf("%5d ", token.line);
      line = token.line;
    } else {
      printf("    | ");
    }
    if (token.kind == TOKEN_KIND_SP_EOL) {
      puts("01 <EOL>");
      continue;
    }
    if (token.kind == TOKEN_KIND_SP_EOF) {
      puts("00 <EOF>");
      break;
    }
    printf("%02d %.*s\n", token.kind, token.length, token.start);
  }
}

static void repl() {
  char line[1024];
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = malloc(file_size + 1);
  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}

void run_file(const char *path) {
  char *source = readFile(path);
  interpret(source);
  free(source);
}

int main(int argc, const char *argv[]) {
  struct scanner scanner;
  struct parser parser;
  struct ast_arena arena;

  scanner_init(&scanner, "\"st\" & \"ri\" & \"ng\"");
  ast_arena_new(&arena);
  parser_init(&parser, &arena, &scanner);

  struct ast *ast = parser_parse(&parser);
  ast_print(ast);
  puts("\n");

  struct vm vm;
  vm_init(&vm);

  chunk_t chunk;
  chunk_init(&chunk);
  chunk_write_from_ast(&chunk, ast, &vm.objects, &vm.strings);
  chunk_write(&chunk, OPCODE_RETURN, 2);

  vm_interpret(&vm, chunk);

  ast_arena_free(&arena);

  return 0;
}
