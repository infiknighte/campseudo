#ifndef CAMPSEUDO_PARSER_H
#define CAMPSEUDO_PARSER_H

#include "arena.h"
#include "ast.h"
#include "scanner.h"

struct parser {
  bool had_error;
  bool panic_mode;
  struct token current;
  struct arena *arena;
  struct scanner *scanner;
};

enum precedence : uint8_t {
  PRECEDENCE_NONE,
  PRECEDENCE_ASSIGNMENT,
  PRECEDENCE_OR,
  PRECEDENCE_AND,
  PRECEDENCE_EQUALITY,
  PRECEDENCE_COMPARISON,
  PRECEDENCE_TERM,
  PRECEDENCE_FACTOR,
  PRECEDENCE_UNARY,
  PRECEDENCE_CALL,
  PRECEDENCE_PRIMARY
};

struct ast *parser_parse(struct parser *parser, struct arena **arena,
                         const char *name);

static inline void parser_init(struct parser *parser, struct scanner *scanner) {
  *parser = (struct parser){
      .scanner = scanner,
      .had_error = false,
      .panic_mode = false,
  };
}

#endif