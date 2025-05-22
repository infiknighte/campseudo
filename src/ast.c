#include "ast.h"
#include "memory.h"
#include <stdint.h>
#include <stdlib.h>

#define CAPACITY_INIT 1024U
#define CAPACITY_GROW(x) ((x) * 3U / 2U)

static void _block_new(ast_block_t *block, uint32_t size) {
  *block =
      reallocate(NULL, 0, sizeof(struct ast_block) + size * sizeof(struct ast));
  **block =
      (struct ast_block){.size = size, .next = NULL, .offset = (*block)->data};
}

void ast_arena_new(struct ast_arena *arena) {
  _block_new(&arena->blocks, CAPACITY_INIT);
  arena->current = arena->blocks;
}

struct ast *ast_arena_make(struct ast_arena *arena) {
  if (arena->current->offset - arena->current->data >= arena->current->size) {
    uint32_t new_size = CAPACITY_GROW(arena->current->size);
    _block_new(&arena->current->next, new_size);
    arena->current = arena->current->next;
  }
  return arena->current->offset++;
}

static void _block_free(ast_block_t block) {
  while (block) {
    ast_block_t next = block->next;
    reallocate(block,
               sizeof(struct ast_block) + block->size * sizeof(struct ast), 0);
    block = next;
  }
}

void ast_arena_free(struct ast_arena *arena) {
  _block_free(arena->blocks);
  arena->blocks = NULL;
  arena->current = NULL;
}

#ifdef DEBUG_AST

#include <stdio.h>

static const char *node_kind_to_str(enum node_kind kind) {
  switch (kind) {
  case NODE_KIND_BOOL:
    return "BOOL";
  case NODE_KIND_CHAR:
    return "CHAR";
  case NODE_KIND_GROUP:
    return "GROUP";
  case NODE_KIND_REAL:
    return "REAL";
  case NODE_KIND_INTEGER:
    return "INTEGER";
  case NODE_KIND_NOT:
    return "NOT";
  case NODE_KIND_NEGATE:
    return "-";
  case NODE_KIND_POINTER:
    return "^";
  case NODE_KIND_ADD:
    return "+";
  case NODE_KIND_SUB:
    return "-";
  case NODE_KIND_MUL:
    return "*";
  case NODE_KIND_DIV:
    return "/";
  case NODE_KIND_INT_DIV:
    return "DIV";
  case NODE_KIND_MOD:
    return "MOD";
  case NODE_KIND_AND:
    return "AND";
  case NODE_KIND_OR:
    return "OR";
  case NODE_KIND_CONCAT:
    return "&";
  case NODE_KIND_EQUAL:
    return "=";
  case NODE_KIND_NOT_EQUAL:
    return "<>";
  case NODE_KIND_GREATER:
    return ">";
  case NODE_KIND_GREATER_EQUAL:
    return ">=";
  case NODE_KIND_LESS:
    return "<";
  case NODE_KIND_LESS_EQUAL:
    return "<=";
  default:
    return "UNKNOWN";
  }
}

void ast_print(const struct ast *ast) {
  if (!ast) {
    fputs("<NULL>", stderr);
    return;
  }

  switch (ast->kind) {
  case NODE_KIND_BOOL:
    fputs(ast->as.boolean ? "TRUE" : "FALSE", stderr);
    break;
  case NODE_KIND_REAL:
    fprintf(stderr, "%f", ast->as.real);
    break;
  case NODE_KIND_INTEGER:
    fprintf(stderr, "%lli", ast->as.integer);
    break;
  case NODE_KIND_CHAR:
    fprintf(stderr, "'%c'", ast->as.cha);
    break;
  case NODE_KIND_STRING:
    fprintf(stderr, "%.*s", ast->as.string.length, ast->as.string.chars);
    break;
  case NODE_KIND_GROUP:
    fputc('(', stderr);
    ast_print(ast->as.expr);
    fputc(')', stderr);
    break;
  case NODE_KIND_NOT:
  case NODE_KIND_NEGATE:
  case NODE_KIND_POINTER:
    printf("(%s ", node_kind_to_str(ast->kind));
    ast_print(ast->as.expr);
    fputc(')', stderr);
    break;
  case NODE_KIND_ADD:
  case NODE_KIND_SUB:
  case NODE_KIND_MUL:
  case NODE_KIND_DIV:
  case NODE_KIND_INT_DIV:
  case NODE_KIND_MOD:
  case NODE_KIND_AND:
  case NODE_KIND_OR:
  case NODE_KIND_CONCAT:
  case NODE_KIND_EQUAL:
  case NODE_KIND_NOT_EQUAL:
  case NODE_KIND_GREATER:
  case NODE_KIND_GREATER_EQUAL:
  case NODE_KIND_LESS:
  case NODE_KIND_LESS_EQUAL:
    fputc('(', stderr);
    ast_print(ast->as.binary.lhs);
    fprintf(stderr, " %s ", node_kind_to_str(ast->kind));
    ast_print(ast->as.binary.rhs);
    fputc(')', stderr);
    break;
  }
}

#endif