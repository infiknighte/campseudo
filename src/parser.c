#include "parser.h"
#include "arena.h"
#include "ast.h"
#include "scanner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ast *(*parse_fn_t)(struct parser *);

struct parse_rule {
  parse_fn_t prefix;
  parse_fn_t infix;
  enum precedence precedence;
};

static const struct parse_rule g_RULES[];

static void _error_at(struct parser *parser, struct token *token,
                      const char *message) {
  if (parser->panic_mode) {
    return;
  }

  parser->had_error = true;
  parser->panic_mode = true;

  fprintf(stderr, "[line %d] Error ", token->line);
  switch (token->kind) {
  case TOKEN_KIND_SP_EOF:
    fputs("at end of file", stderr);
    break;
  case TOKEN_KIND_SP_EOL:
    fputs("at end of line", stderr);
  case TOKEN_KIND_SP_ERROR:
    break;
  default:
    fprintf(stderr, "at '%.*s'", token->length, token->start);
    break;
  }

  fprintf(stderr, ": %s\n", message);
}

static inline void _error(struct parser *parser, const char *message) {
  _error_at(parser, &parser->current, message);
}

static inline void _advance(struct parser *parser) {
  parser->current = scanner_scan_token(parser->scanner);
}

static inline bool _check(const struct parser *parser, enum token_kind kind) {
  return parser->current.kind == kind;
}

static inline void _consume(struct parser *parser, enum token_kind expect,
                            const char *message) {
  if (_check(parser, expect)) {
    _advance(parser);
  } else {
    _error(parser, message);
  }
}

static inline bool _match(struct parser *parser, enum token_kind kind) {
  if (_check(parser, kind)) {
    _advance(parser);
    return true;
  }
  return false;
}

static struct ast *_prefix_char(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  *node = (struct ast){parser->current.line, NODE_KIND_CHAR,
                       .as.cha = atoi(parser->current.start)};
  _advance(parser);
  return node;
}

static struct ast *_prefix_true(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  *node =
      (struct ast){parser->current.line, NODE_KIND_BOOL, .as.boolean = true};
  _advance(parser);
  return node;
}

static struct ast *_prefix_false(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  *node =
      (struct ast){parser->current.line, NODE_KIND_BOOL, .as.boolean = false};
  _advance(parser);
  return node;
}

static struct ast *_prefix_integer(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  *node = (struct ast){parser->current.line, NODE_KIND_INTEGER,
                       .as.integer = strtoll(parser->current.start, NULL, 10)};
  _advance(parser);
  return node;
}

static struct ast *_prefix_real(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  *node = (struct ast){parser->current.line, NODE_KIND_REAL,
                       .as.real = strtod(parser->current.start, NULL)};
  _advance(parser);
  return node;
}

static const enum node_kind g_OP_NODE_KIND[] = {
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
  _consume(parser, TOKEN_KIND_OP_PAREN_CLOSE, "Expect ')' after expression.");
  return expr;
}

static struct ast *_prefix_negate(struct parser *parser) {
  struct ast *negation = ARENA_ALLOC(parser->arena, struct ast);
  _advance(parser);
  negation->line = parser->current.line;
  negation->kind = NODE_KIND_NEGATE;
  negation->as.expr = _parse_precedence(parser, PRECEDENCE_UNARY);
  return negation;
}

static struct ast *_prefix_not(struct parser *parser) {
  struct ast *expr = ARENA_ALLOC(parser->arena, struct ast);
  _advance(parser);
  expr->line = parser->current.line;
  expr->kind = NODE_KIND_NOT;
  expr->as.expr = _parse_precedence(parser, PRECEDENCE_UNARY);
  return expr;
}

static inline struct ast *_prefix_positive(struct parser *parser) {
  _advance(parser);
  return _parse_precedence(parser, PRECEDENCE_UNARY);
}

static struct ast *_prefix_string(struct parser *parser) {
  struct ast *node = ARENA_ALLOC(parser->arena, struct ast);
  struct token token = parser->current;
  *node = (struct ast){token.line, NODE_KIND_STRING,
                       .as.string = {token.length, (uint8_t *)token.start}};
  _advance(parser);
  return node;
}

static struct ast *_prefix_variable(struct parser *parser) {
  struct ast *ident = ARENA_ALLOC(parser->arena, struct ast);
  *ident = (struct ast){
      .kind = NODE_KIND_VAR_GET,
      .as.ident = {parser->current.length, (uint8_t *)parser->current.start},
      .line = parser->current.line};
  _advance(parser);
  return ident;
}

static inline struct ast *_infix_binary(struct parser *parser) {
  _advance(parser);
  return _parse_precedence(parser,
                           g_RULES[parser->current.kind].precedence + 1);
}

static struct ast *_parse_precedence(struct parser *parser,
                                     enum precedence precedence) {
  struct token current = parser->current;

  const parse_fn_t prefix_rule = g_RULES[current.kind].prefix;
  if (!prefix_rule) {
    _error(parser, "Expect expression");
  }

  struct ast *expr = prefix_rule(parser);

  while (precedence <= g_RULES[(current = parser->current).kind].precedence) {
    parse_fn_t infix_rule = g_RULES[current.kind].infix;
    struct ast *lhs = expr;
    expr = ARENA_ALLOC(parser->arena, struct ast);
    *expr = (struct ast){current.line, g_OP_NODE_KIND[current.kind],
                         .as.binary = {lhs, infix_rule(parser)}};
  }

  return expr;
}

static inline struct ast *_expression(struct parser *parser) {
  return _parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

static const struct parse_rule g_RULES[] = {
    [TOKEN_KIND_SP_IDENT] = {_prefix_variable, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_CHAR] = {_prefix_char, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_DATE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_FALSE] = {_prefix_false, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_INTEGER] = {_prefix_integer, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_REAL] = {_prefix_real, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_STRING] = {_prefix_string, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_LT_TRUE] = {_prefix_true, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_ADDITION] = {_prefix_positive, _infix_binary,
                                PRECEDENCE_TERM},
    [TOKEN_KIND_OP_CONCAT] = {NULL, _infix_binary, PRECEDENCE_TERM},
    [TOKEN_KIND_OP_DIVISION] = {NULL, _infix_binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_OP_DOT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_EQUAL_TO] = {NULL, _infix_binary, PRECEDENCE_EQUALITY},
    [TOKEN_KIND_OP_GREATER_OR_EQUAL_TO] = {NULL, _infix_binary,
                                           PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_GREATER_THAN] = {NULL, _infix_binary, PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_LESS_OR_EQUAL_TO] = {NULL, _infix_binary,
                                        PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_LESS_THAN] = {NULL, _infix_binary, PRECEDENCE_COMPARISON},
    [TOKEN_KIND_OP_MULTIPLICATION] = {NULL, _infix_binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_OP_NOT_EQUAL_TO] = {NULL, NULL, PRECEDENCE_EQUALITY},
    [TOKEN_KIND_OP_PAREN_OPEN] = {_group, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_OP_SUBTRACTION] = {_prefix_negate, _infix_binary,
                                   PRECEDENCE_TERM},
    [TOKEN_KIND_KW_AND] = {NULL, _infix_binary, PRECEDENCE_AND},
    [TOKEN_KIND_KW_DIV] = {NULL, _infix_binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_KW_MOD] = {NULL, _infix_binary, PRECEDENCE_FACTOR},
    [TOKEN_KIND_KW_NOT] = {_prefix_not, NULL, PRECEDENCE_NONE},
    [TOKEN_KIND_KW_OR] = {NULL, _infix_binary, PRECEDENCE_OR},
};

static struct ast *_statement_expression(struct parser *parser) {
  struct ast *stmt = ARENA_ALLOC(parser->arena, struct ast);
  *stmt =
      (struct ast){.kind = NODE_KIND_EXPR_STMT, .line = parser->current.line};
  stmt->as.expr = _expression(parser);
  _consume(parser, TOKEN_KIND_SP_EOL,
           "Expect newline ('\\n') after statement expression.");
  return stmt;
}

static struct ast *_statement_output(struct parser *parser) {
  struct ast *stmt = ARENA_ALLOC(parser->arena, struct ast);
  *stmt =
      (struct ast){.kind = NODE_KIND_OUTPUT_STMT, .line = parser->current.line};
  stmt->as.expr = _expression(parser);
  _consume(parser, TOKEN_KIND_SP_EOL,
           "Expect newline ('\\n') after output statement.");
  return stmt;
}

static struct ast *_statement(struct parser *parser) {
  if (_match(parser, TOKEN_KIND_KW_OUTPUT)) {
    return _statement_output(parser);
  }
  return _statement_expression(parser);
}

static void _synchronize(struct parser *parser) {
  parser->panic_mode = false;

  while (!_check(parser, TOKEN_KIND_SP_EOF)) {
    switch (parser->current.kind) {
    case TOKEN_KIND_SP_EOL:
      _advance(parser);
      return;
    case TOKEN_KIND_KW_AND:
    case TOKEN_KIND_KW_APPEND:
    case TOKEN_KIND_KW_CALL:
    case TOKEN_KIND_KW_CASE:
    case TOKEN_KIND_KW_CLASS:
    case TOKEN_KIND_KW_CLOSEFILE:
    case TOKEN_KIND_KW_CONSTANT:
    case TOKEN_KIND_KW_DECLARE:
    case TOKEN_KIND_KW_DEFINE:
    case TOKEN_KIND_KW_FOR:
    case TOKEN_KIND_KW_FUNCTION:
    case TOKEN_KIND_KW_GETRECORD:
    case TOKEN_KIND_KW_IF:
    case TOKEN_KIND_KW_INPUT:
    case TOKEN_KIND_KW_OPENFILE:
    case TOKEN_KIND_KW_OTHERWISE:
    case TOKEN_KIND_KW_OUTPUT:
    case TOKEN_KIND_KW_PROCEDURE:
    case TOKEN_KIND_KW_PUTRECORD:
    case TOKEN_KIND_KW_READFILE:
    case TOKEN_KIND_KW_READ:
    case TOKEN_KIND_KW_REPEAT:
    case TOKEN_KIND_KW_RETURN:
    case TOKEN_KIND_KW_SEEK:
    case TOKEN_KIND_KW_SET:
    case TOKEN_KIND_KW_TYPE:
    case TOKEN_KIND_KW_UNTIL:
    case TOKEN_KIND_KW_WHILE:
    case TOKEN_KIND_KW_WRITEFILE:
    case TOKEN_KIND_KW_WRITE:
      return;
    default:
      break;
    }

    _advance(parser);
  }
}

static struct ast *_declare_variable(struct parser *parser) {
  struct ast *decl = ARENA_ALLOC(parser->arena, struct ast);
  *decl =
      (struct ast){.kind = NODE_KIND_VAR_DECL,
                   .as.var.ident = {.start = (uint8_t *)parser->current.start,
                                    .length = parser->current.length},
                   .line = parser->current.line};

  _consume(parser, TOKEN_KIND_SP_IDENT,
           "Expect identfier after keyword 'DECLARE'");

  decl->as.var.expr =
      (_match(parser, TOKEN_KIND_OP_ASSIGN)) ? _expression(parser) : NULL;

  _consume(parser, TOKEN_KIND_SP_EOL,
           "Expect newline ('\\n') after variable declaration.");

  return decl;
}

static struct ast *_declaration(struct parser *parser) {
  struct ast *result;

  if (_match(parser, TOKEN_KIND_KW_DECLARE)) {
    result = _declare_variable(parser);
  } else {
    result = _statement(parser);
  }

  if (parser->panic_mode) {
    _synchronize(parser);
  }

  return result;
}

static struct ast *_end(struct arena *arena, uint32_t line) {
  struct ast *end = ARENA_ALLOC(arena, struct ast);
  end->kind = NODE_KIND_END;
  end->line = line;
  return end;
}

static struct ast *_program(struct parser *parser, const char *name) {
  struct ast *ast = ARENA_ALLOC(parser->arena, struct ast);
  ast->kind = NODE_KIND_PROGRAM;
  ast->as.program.name = name;

  struct ast_array *decls = ast_array_new();
  while (!_match(parser, TOKEN_KIND_SP_EOF)) {
    ast_array_push(&decls, _declaration(parser));
  }

  ast_array_push(&decls,
                 _end(parser->arena, decls->ptr[decls->count - 1]->line + 1));

  ast->as.program.decls = decls;
  return ast;
}

struct ast *parser_parse(struct parser *parser, struct arena **arena,
                         const char *name) {
  parser->arena = *arena;

  _advance(parser);
  struct ast *program = _program(parser, name);

  *arena = parser->arena;

  return (parser->had_error) ? NULL : program;
}
