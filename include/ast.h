#ifndef COMPSEUDO_AST_H
#define COMPSEUDO_AST_H

#include "common.h"
#include <stdint.h>
#include <stdlib.h>

enum node_kind : uint8_t {
  NODE_KIND_PROGRAM,
  NODE_KIND_OUTPUT_STMT,
  NODE_KIND_EXPR_STMT,
  NODE_KIND_END,

  // Terminals
  NODE_KIND_BOOL,
  NODE_KIND_CHAR,
  NODE_KIND_REAL,
  NODE_KIND_INTEGER,
  NODE_KIND_STRING,
  NODE_KIND_VAR_DECL,
  NODE_KIND_VAR_GET,

  // Unary Expresions
  NODE_KIND_NOT,
  NODE_KIND_NEGATE,

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
      const uint8_t *chars;
    } string, ident;

    struct ast *expr;

    struct {
      struct ast *lhs;
      struct ast *rhs;
    } binary;

    struct {
      struct ast *expr;
      struct {
        uint32_t length;
        const uint8_t *start;
      } ident;
    } var;

    struct {
      const char *name;
      struct ast_array *decls;
    } program;

  } as;
};

struct ast_array {
  uint64_t count, capacity;
  struct ast *ptr[];
};

struct ast_array *ast_array_new(void);
void ast_array_push(struct ast_array **array, struct ast *ast);

static inline void ast_array_free(struct ast_array *array) { free(array); }

#ifdef DEBUG_AST
void ast_print(const struct ast *ast);
#endif

#endif