#include "parser.h"
#include "ast.h"
#include "scanner.h"
#include <stdlib.h>

typedef struct ast *(*parse_fn_t)(struct parser *);

struct parse_rule {
  parse_fn_t prefix;
  parse_fn_t infix;
  enum precedence precedence;
};

static const struct parse_rule g_RULES[];

static inline void _advance(struct parser *parser) {
  parser->current = scanner_scan_token(parser->scanner);
}

static inline void _consume(struct parser *parser, enum token_kind expect) {
  if (parser->current.kind != expect) {
  }
  _advance(parser);
}

static struct ast *_prefix_char(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  *node = (struct ast){parser->current.line, NODE_KIND_CHAR,
                       .as.cha = atoi(parser->current.start)};
  _advance(parser);
  return node;
}

static struct ast *_prefix_true(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  *node =
      (struct ast){parser->current.line, NODE_KIND_BOOL, .as.boolean = true};
  _advance(parser);
  return node;
}

static struct ast *_prefix_false(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  *node =
      (struct ast){parser->current.line, NODE_KIND_BOOL, .as.boolean = false};
  _advance(parser);
  return node;
}

static struct ast *_prefix_integer(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  *node = (struct ast){parser->current.line, NODE_KIND_INTEGER,
                       .as.integer = strtoll(parser->current.start, NULL, 10)};
  _advance(parser);
  return node;
}

static struct ast *_prefix_real(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  *node = (struct ast){parser->current.line, NODE_KIND_REAL,
                       .as.real = strtod(parser->current.start, NULL)};
  _advance(parser);
  return node;
}

static const enum node_kind g_NODE_KIND[] = {
    [TOKEN_KIND_OP_CONCAT] = NODE_KIND_CONCAT,
    [TOKEN_KIND_OP_ADDITION] = NODE_KIND_ADD,
    [TOKEN_KIND_KW_AND] = NODE_KIND_AND,
    [TOKEN_KIND_KW_OR] = NODE_KIND_OR,
    [TOKEN_KIND_OP_DIVISION] = NODE_KIND_DIV,
    [TOKEN_KIND_KW_DIV] = NODE_KIND_INT_DIV,
    [TOKEN_KIND_OP_MULTIPLICATION] = NODE_KIND_MUL,
    [TOKEN_KIND_OP_SUBTRACTION] = NODE_KIND_SUB,
    [TOKEN_KIND_KW_MOD] = NODE_KIND_MOD,
    [TOKEN_KIND_OP_EQUAL_TO] = NODE_KIND_EQUAL,
    [TOKEN_KIND_OP_GREATER_OR_EQUAL_TO] = NODE_KIND_GREATER_EQUAL,
    [TOKEN_KIND_OP_GREATER_THAN] = NODE_KIND_GREATER,
    [TOKEN_KIND_OP_LESS_OR_EQUAL_TO] = NODE_KIND_LESS_EQUAL,
    [TOKEN_KIND_OP_LESS_THAN] = NODE_KIND_LESS,
    [TOKEN_KIND_OP_NOT_EQUAL_TO] = NODE_KIND_NOT_EQUAL,
};

static struct ast *_expression(struct parser *parser);
static struct ast *_parse_precedence(struct parser *parser,
                                     enum precedence precedence);

static struct ast *_group(struct parser *parser) {
  _advance(parser);
  struct ast *expr = _expression(parser);
  _consume(parser, TOKEN_KIND_OP_PAREN_CLOSE);
  return expr;
}

static struct ast *_unary(struct parser *parser) {
  struct token token = parser->current;
  struct ast *expr = ast_arena_make(parser->arena);
  _advance(parser);
  expr->line = token.line;
  switch (token.kind) {
  case TOKEN_KIND_KW_NOT:
    expr->kind = NODE_KIND_NOT;
    break;
  case TOKEN_KIND_OP_SUBTRACTION:
    expr->kind = NODE_KIND_NEGATE;
    break;
  default:
    break;
  }
  expr->as.expr = _parse_precedence(parser, PRECEDENCE_UNARY);
  return expr;
}

static struct ast *_prefix_string(struct parser *parser) {
  struct ast *node = ast_arena_make(parser->arena);
  struct token token = parser->current;
  *node = (struct ast){token.line, NODE_KIND_STRING,
                       .as.string = {token.length, token.start}};
  _advance(parser);
  return node;
}

static struct ast *_binary(struct parser *parser) {
  enum precedence precedence = g_RULES[parser->current.kind].precedence;
  _advance(parser);
  struct ast *expr = _parse_precedence(parser, precedence + 1);
  return expr;
}

static struct ast *_parse_precedence(struct parser *parser,
                                     enum precedence precedence) {
  const parse_fn_t prefix_rule = g_RULES[parser->current.kind].prefix;
  if (!prefix_rule) {
  }

  struct ast *expr = prefix_rule(parser);

  struct token current;
  while (precedence <= g_RULES[(current = parser->current).kind].precedence) {
    parse_fn_t infix_rule = g_RULES[current.kind].infix;
    struct ast *lhs = expr;
    expr = ast_arena_make(parser->arena);
    *expr = (struct ast){current.line, g_NODE_KIND[current.kind],
                         .as.binary = {lhs, infix_rule(parser)}};
  }

  return expr;
}

static struct ast *_expression(struct parser *parser) {
  return _parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

static const struct parse_rule g_RULES[] = {
    [TOKEN_KIND_SP_IDENT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_CHAR] = {_prefix_char, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_DATE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_FALSE] = {_prefix_false, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_INTEGER] = {_prefix_integer, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_REAL] = {_prefix_real, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_STRING] = {_prefix_string, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_TRUE] = {_prefix_true, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_ADDITION] = {NULL, _binary, PRECEDENCE_TERM},
    [TOKEN_KIND_OP_CONCAT] = {NULL, _binary, PRECEDENCE_TERM},
    [TOKEN_KIND_OP_DIVISION] = {NULL, _binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_OP_DOT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_EQUAL_TO] = {NULL, _binary, PRECEDENCE_EQUALITY},
    [TOKEN_KIND_OP_GREATER_OR_EQUAL_TO] = {NULL, _binary,
                                           PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_GREATER_THAN] = {NULL, _binary, PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_LESS_OR_EQUAL_TO] = {NULL, _binary, PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_LESS_THAN] = {NULL, _binary, PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_MULTIPLICATION] = {NULL, _binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_OP_NOT_EQUAL_TO] = {NULL, NULL, PRECEDENCE_EQUALITY},
    [TOKEN_KIND_OP_PAREN_OPEN] = {_group, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_SUBTRACTION] = {_unary, _binary, PRECEDENCE_TERM},
    [TOKEN_KIND_KW_AND] = {NULL, _binary, PRECEDENCE_AND},
    [TOKEN_KIND_KW_DIV] = {NULL, _binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_KW_MOD] = {NULL, _binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_KW_NOT] = {_unary, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_KW_OR] = {NULL, _binary, PRECEDENCE_OR},
};

struct ast *parser_parse(struct parser *parser) {
  struct ast *ast = _expression(parser);
  _consume(parser, TOKEN_KIND_SP_EOF);
  return ast;
}

void parser_init(struct parser *parser, struct ast_arena *arena,
                 struct scanner *scanner) {
  parser->arena = arena;
  parser->scanner = scanner;
  parser->had_error = false;
  _advance(parser);
}
