#ifndef CAMPSEUDO_PARSER_H
#define CAMPSEUDO_PARSER_H

#include "ast.h"
#include "scanner.h"

struct parser {
  bool had_error;
  struct token current;
  struct ast_arena *arena;
  struct scanner *scanner;
};

enum precedence {
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

void parser_init(struct parser *parser, struct ast_arena *arena,
                 struct scanner *scanner);

struct ast *parser_parse(struct parser *parser);

#endif