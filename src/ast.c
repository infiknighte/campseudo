#include "ast.h"

struct ast_array *ast_array_new(void) {
  struct ast_array *array =
      malloc(sizeof(struct ast_array) + 64 * sizeof(struct ast));
  *array = (struct ast_array){.count = 0, .capacity = 64};
  return array;
}

void ast_array_push(struct ast_array **array, struct ast *ast) {
  struct ast_array *arr = *array;
  if (arr->count >= arr->capacity) {
    uint64_t new_capacity = arr->capacity * 2;
    *array = arr = realloc(arr, sizeof(struct ast_array) +
                                    new_capacity * sizeof(struct ast *));
    arr->capacity = new_capacity;
  }
  arr->ptr[arr->count++] = ast;
}

#ifdef DEBUG_AST

#include <stdio.h>

static const char *node_kind_to_str(enum node_kind kind) {
  switch (kind) {
  case NODE_KIND_PROGRAM:
    return "MODULE";
  case NODE_KIND_BOOL:
    return "BOOL";
  case NODE_KIND_CHAR:
    return "CHAR";
  case NODE_KIND_REAL:
    return "REAL";
  case NODE_KIND_INTEGER:
    return "INTEGER";
  case NODE_KIND_NOT:
    return "NOT";
  case NODE_KIND_NEGATE:
    return "-";
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
    fprintf(stderr, "%lld", ast->as.integer);
    break;
  case NODE_KIND_CHAR:
    fprintf(stderr, "'%c'", ast->as.cha);
    break;
  case NODE_KIND_STRING:
    fprintf(stderr, "%.*s", ast->as.string.length, ast->as.string.chars);
    break;
  case NODE_KIND_NOT:
  case NODE_KIND_NEGATE:
    fprintf(stderr, "(%s ", node_kind_to_str(ast->kind));
    ast_print(ast->as.expr);
    fputc(')', stderr);
    break;
  case NODE_KIND_PROGRAM: {
    fprintf(stderr, "PROGRAM \'%s\'\n", ast->as.program.name);
    struct ast_array *decls = ast->as.program.decls;
    for (uint32_t i = 0; i < decls->count; ++i) {
      ast_print(decls->ptr[i]);
    }
    break;
  }
  case NODE_KIND_VAR_DECL: {
    fprintf(stderr, "%d| DECLARE %.*s", ast->line, ast->as.var.ident.length,
            ast->as.var.ident.start);
    if (ast->as.var.expr) {
      fputs(" <- ", stderr);
      ast_print(ast->as.var.expr);
    }
    putc('\n', stderr);
    break;
  }
  case NODE_KIND_VAR_GET:
    fprintf(stderr, "%.*s", ast->as.ident.length, ast->as.ident.chars);
    break;
  case NODE_KIND_OUTPUT_STMT: {
    fprintf(stderr, "%d| OUTPUT ", ast->line);
    ast_print(ast->as.expr);
    putc('\n', stderr);
    break;
  }
  case NODE_KIND_EXPR_STMT:
    fprintf(stderr, "%d| ", ast->line);
    ast_print(ast->as.expr);
    putc('\n', stderr);
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
  case NODE_KIND_END:
    fprintf(stderr, "%d| END\n", ast->line);
    break;
  default:
    fprintf(stderr, "Unknown Node Kind found %d", ast->kind);
    break;
  }
}

#endif