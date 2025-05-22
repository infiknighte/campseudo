#ifndef COMPSEUDO_AST_H
#define COMPSEUDO_AST_H

#include "common.h"
#include <stdint.h>

#define AST_NODE()

enum node_kind : uint8_t {

  // Terminals
  NODE_KIND_BOOL,
  NODE_KIND_CHAR,
  NODE_KIND_REAL,
  NODE_KIND_INTEGER,
  NODE_KIND_STRING,

  // Unary Expresions
  NODE_KIND_NOT,
  NODE_KIND_NEGATE,
  NODE_KIND_POINTER,
  NODE_KIND_GROUP,

  // Binary Expresions
  NODE_KIND_ADD,
  NODE_KIND_SUB,
  NODE_KIND_MUL,
  NODE_KIND_DIV,
  NODE_KIND_INT_DIV,
  NODE_KIND_MOD,
  NODE_KIND_AND,
  NODE_KIND_OR,
  NODE_KIND_CONCAT,
  NODE_KIND_EQUAL,
  NODE_KIND_NOT_EQUAL,
  NODE_KIND_GREATER,
  NODE_KIND_GREATER_EQUAL,
  NODE_KIND_LESS,
  NODE_KIND_LESS_EQUAL,

};

struct ast {
  uint32_t line;
  enum node_kind kind;
  union {
    bool boolean;
    uint8_t cha;
    double real;
    int64_t integer;
    struct {
      uint32_t length;
      const char *chars;
    } string;

    struct ast *expr;

    struct {
      struct ast *lhs;
      struct ast *rhs;
    } binary;
  } as;
};

typedef struct ast_block {
  uint32_t size;
  struct ast *offset;
  struct ast_block *next;
  struct ast data[];
} *ast_block_t;

struct ast_arena {
  struct ast_block *current;
  ast_block_t blocks;
};

void ast_arena_new(struct ast_arena *arena);
struct ast *ast_arena_make(struct ast_arena *arena);
void ast_arena_free(struct ast_arena *arena);

#ifdef DEBUG_AST
void ast_print(const struct ast *ast);
#endif

#endif