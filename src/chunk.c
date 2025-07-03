#include "chunk.h"
#include "memory.h"
#include "obj.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>

#define UINT24_MAX 0xffffff

#define CAPACITY_INIT 8U
#define CAPACITY_MULT 2U

static void _line_write(struct line_array **array, uint32_t line) {
  struct line_array *self = *array;

  uint32_t count = self->count;
  if (count > 1 && line == self->lines[count - 2]) {
    self->lines[count - 1]++;
    return;
  }

  if (self->capacity < count + 2) {
    uint32_t new_capacity = (*array)->capacity * CAPACITY_MULT;
    void *temp = reallocate(
        *array,
        sizeof(struct line_array) + (*array)->capacity * sizeof(uint32_t),
        sizeof(struct line_array) + new_capacity * sizeof(uint32_t));
    if (!temp) {
      free(*array);
      exit(1);
    }
    *array = self = temp;
    self->capacity = new_capacity;
  }

  self->lines[count++] = line;
  self->lines[count++] = 1;
}

struct line_array *line_array_new(void) {
  struct line_array *array =
      MEM_ALLOC(sizeof(struct line_array) + sizeof(uint32_t) * CAPACITY_INIT);
  array->count = 0;
  array->capacity = CAPACITY_INIT;
  return array;
}

struct chunk *chunk_new(void) {
  struct chunk *chunk = MEM_ALLOC(sizeof(struct chunk) + CAPACITY_INIT);
  *chunk = (struct chunk){
      .count = 0,
      .capacity = CAPACITY_INIT,
      .lines = line_array_new(),
      .constants = value_array_new(),
  };
  return chunk;
}

void chunk_free(struct chunk *chunk) {
  value_array_free(chunk->constants);
  MEM_FREE(chunk->lines, sizeof(struct line_array) +
                             sizeof(uint32_t) * chunk->lines->capacity);
  MEM_FREE(chunk, sizeof(struct chunk) + CAPACITY_INIT);
}

void chunk_write(struct chunk **chunk, enum opcode byte, uint32_t line) {
  if ((*chunk)->capacity < (*chunk)->count + 1) {
    uint64_t new_capcity = (*chunk)->capacity * CAPACITY_MULT;
    *chunk = reallocate(*chunk, sizeof(struct chunk) + (*chunk)->capacity,
                        sizeof(struct chunk) + new_capcity);
    (*chunk)->capacity = new_capcity;
  }
  (*chunk)->code[(*chunk)->count++] = byte;
  _line_write(&(*chunk)->lines, line);
}

uint32_t chunk_add_constant(struct chunk *chunk, struct value value) {
  value_array_write(&chunk->constants, value);
  return chunk->constants->count - 1;
}

uint32_t chunk_write_constant(struct chunk **chunk, struct value value,
                              uint32_t line) {

  uint64_t count = (*chunk)->constants->count;
  uint64_t constant = chunk_add_constant(*chunk, value);

  if (count < UINT8_MAX) {
    chunk_write(chunk, OPCODE_CONSTANT_8, line);
    chunk_write(chunk, constant & 0xff, line);
  } else if (count < UINT16_MAX) {
    chunk_write(chunk, OPCODE_CONSTANT_16, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
  } else if (count < UINT24_MAX) {
    chunk_write(chunk, OPCODE_CONSTANT_24, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
    chunk_write(chunk, constant & 0xff0000, line);
  } else {
  }

  return constant;
}

uint32_t chunk_get_line(const struct chunk *chunk, uint32_t index) {
  struct line_array *array = chunk->lines;
  uint32_t n = 0;
  for (uint32_t i = 1; i < array->count; i += 2) {
    if (index < (n += array->lines[i])) {
      return array->lines[i - 1];
    }
  }
  return 0;
}

uint32_t _define_variable(struct chunk **chunk, struct value value,
                          uint32_t line) {

  uint32_t count = (*chunk)->constants->count;
  uint32_t constant = chunk_add_constant(*chunk, value);

  if (count < UINT8_MAX) {
    chunk_write(chunk, OPCODE_DEFINE_GLOBAL_8, line);
    chunk_write(chunk, constant & 0xff, line);
  } else if (count < UINT16_MAX) {
    chunk_write(chunk, OPCODE_DEFINE_GLOBAL_16, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
  } else if (count < UINT24_MAX) {
    chunk_write(chunk, OPCODE_DEFINE_GLOBAL_24, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
    chunk_write(chunk, constant & 0xff0000, line);
  } else {
  }

  return constant;
}

uint32_t _get_variable(struct chunk **chunk, struct value value,
                       uint32_t line) {

  uint64_t count = (*chunk)->constants->count;
  uint64_t constant = chunk_add_constant(*chunk, value);

  if (count < UINT8_MAX) {
    chunk_write(chunk, OPCODE_GET_GLOBAL_8, line);
    chunk_write(chunk, constant & 0xff, line);
  } else if (count < UINT16_MAX) {
    chunk_write(chunk, OPCODE_GET_GLOBAL_16, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
  } else if (count < UINT24_MAX) {
    chunk_write(chunk, OPCODE_GET_GLOBAL_24, line);
    chunk_write(chunk, constant & 0xff, line);
    chunk_write(chunk, constant & 0xff00, line);
    chunk_write(chunk, constant & 0xff0000, line);
  } else {
  }

  return constant;
}

void chunk_write_ast(struct chunk **chunk, struct ast *ast,
                     struct obj **objects, struct table **strings) {
#define WRITE_VALUE(value) chunk_write_constant(chunk, value, ast->line)
#define WRITE_UNARY(opcode)                                                    \
  chunk_write_ast(chunk, ast->as.expr, objects, strings);                      \
  chunk_write(chunk, opcode, ast->line)

#define WRITE_BINARY(opcode)                                                   \
  chunk_write_ast(chunk, ast->as.binary.lhs, objects, strings);                \
  chunk_write_ast(chunk, ast->as.binary.rhs, objects, strings);                \
  chunk_write(chunk, opcode, ast->line)

  switch (ast->kind) {
  case NODE_KIND_BOOL:
    chunk_write(chunk, ast->as.boolean ? OPCODE_TRUE : OPCODE_FALSE, ast->line);
    break;
  case NODE_KIND_CHAR:
    WRITE_VALUE(VALUE_FROM_CHAR(ast->as.cha));
    break;
  case NODE_KIND_REAL:
    WRITE_VALUE(VALUE_FROM_REAL(ast->as.real));
    break;
  case NODE_KIND_INTEGER:
    WRITE_VALUE(VALUE_FROM_INTEGER(ast->as.integer));
    break;
  case NODE_KIND_STRING:
    WRITE_VALUE(VALUE_FROM_OBJ(obj_string_ref(objects, strings,
                                              ast->as.string.chars + 1,
                                              ast->as.string.length - 2)));
    break;
  case NODE_KIND_NOT:
    WRITE_UNARY(OPCODE_NOT);
    break;
  case NODE_KIND_NEGATE:
    WRITE_UNARY(OPCODE_NEGATE);
    break;
  case NODE_KIND_ADD:
    WRITE_BINARY(OPCODE_ADD);
    break;
  case NODE_KIND_SUB:
    WRITE_BINARY(OPCODE_SUB);
    break;
  case NODE_KIND_MUL:
    WRITE_BINARY(OPCODE_MUL);
    break;
  case NODE_KIND_DIV:
    WRITE_BINARY(OPCODE_DIV);
    break;
  case NODE_KIND_INT_DIV:
    WRITE_BINARY(OPCODE_INT_DIV);
    break;
  case NODE_KIND_MOD:
    WRITE_BINARY(OPCODE_MOD);
    break;
  case NODE_KIND_AND:
    WRITE_BINARY(OPCODE_AND);
    break;
  case NODE_KIND_OR:
    WRITE_BINARY(OPCODE_OR);
    break;
  case NODE_KIND_CONCAT:
    WRITE_BINARY(OPCODE_CONCAT);
    break;
  case NODE_KIND_EQUAL:
    WRITE_BINARY(OPCODE_EQUAL);
    break;
  case NODE_KIND_NOT_EQUAL:
    WRITE_BINARY(OPCODE_NOT_EQUAL);
    break;
  case NODE_KIND_GREATER:
    WRITE_BINARY(OPCODE_GREATER);
    break;
  case NODE_KIND_GREATER_EQUAL:
    WRITE_BINARY(OPCODE_GREATER_EQUAL);
    break;
  case NODE_KIND_LESS:
    WRITE_BINARY(OPCODE_LESS);
    break;
  case NODE_KIND_LESS_EQUAL:
    WRITE_BINARY(OPCODE_LESS_EQUAL);
    break;
  case NODE_KIND_PROGRAM: {
    struct ast_array *decls = ast->as.program.decls;
    for (uint32_t i = 0; i < decls->count; ++i) {
      chunk_write_ast(chunk, decls->ptr[i], objects, strings);
    }
    break;
  }
  case NODE_KIND_VAR_DECL: {
    if (ast->as.var.expr) {
      chunk_write_ast(chunk, ast->as.var.expr, objects, strings);
    } else {
      chunk_write_constant(chunk, VALUE_NONE(), ast->line);
    }
    _define_variable(
        chunk,
        VALUE_FROM_OBJ(obj_string_ref(objects, strings, ast->as.var.ident.start,
                                      ast->as.var.ident.length)),
        ast->line);
    break;
  }
  case NODE_KIND_VAR_GET: {
    _get_variable(
        chunk,
        VALUE_FROM_OBJ(obj_string_ref(objects, strings, ast->as.ident.chars,
                                      ast->as.ident.length)),
        ast->line);
    break;
  }
  case NODE_KIND_OUTPUT_STMT: {
    chunk_write_ast(chunk, ast->as.expr, objects, strings);
    chunk_write(chunk, OPCODE_OUTPUT, ast->line);
    break;
  }
  case NODE_KIND_EXPR_STMT:
    chunk_write_ast(chunk, ast->as.expr, objects, strings);
    chunk_write(chunk, OPCODE_POP, ast->line);
    break;
  case NODE_KIND_END:
    chunk_write(chunk, OPCODE_RETURN, ast->line);
    break;
  }
#undef WRITE_CONSTANT
#undef WRITE_UNARY
#undef WRITE_BINARY
}

#ifdef DEBUG_CHUNK
#include "stdio.h"

static const char *const g_INSTRUCTION_NAME[] = {
    [OPCODE_NONE] = "NONE",
    [OPCODE_CONSTANT_8] = "CONSTANT_8",
    [OPCODE_CONSTANT_16] = "CONSTANT_16",
    [OPCODE_CONSTANT_24] = "CONSTANT_24",
    [OPCODE_DEFINE_GLOBAL_8] = "DEFINE_GLOBAL_8",
    [OPCODE_DEFINE_GLOBAL_16] = "DEFINE_GLOBAL_16",
    [OPCODE_DEFINE_GLOBAL_24] = "DEFINE_GLOBAL_24",
    [OPCODE_GET_GLOBAL_8] = "GET_GLOBAL_8",
    [OPCODE_GET_GLOBAL_16] = "GET_GLOBAL_16",
    [OPCODE_GET_GLOBAL_24] = "GET_GLOBAL_24",
    [OPCODE_POP] = "POP",
    [OPCODE_TRUE] = "TRUE",
    [OPCODE_FALSE] = "FALSE",
    [OPCODE_AND] = "AND",
    [OPCODE_OR] = "OR",
    [OPCODE_ADD] = "ADD",
    [OPCODE_SUB] = "SUB",
    [OPCODE_MUL] = "MUL",
    [OPCODE_DIV] = "DIV",
    [OPCODE_INT_DIV] = "INT_DIV",
    [OPCODE_MOD] = "MOD",
    [OPCODE_CONCAT] = "CONCAT",
    [OPCODE_NEGATE] = "NEGATE",
    [OPCODE_NOT] = "NOT",
    [OPCODE_EQUAL] = "EQUAL",
    [OPCODE_NOT_EQUAL] = "NOT_EQUAL",
    [OPCODE_LESS] = "LESS",
    [OPCODE_LESS_EQUAL] = "LESS_EQUAL",
    [OPCODE_GREATER] = "GREATER",
    [OPCODE_GREATER_EQUAL] = "GREATER_EQUAL",
    [OPCODE_OUTPUT] = "OUTPUT",
    [OPCODE_RETURN] = "RETURN",
};

static inline void _simple_instruction(const char *name) {
  fprintf(stderr, "%s\n", name);
}

static uint32_t _constant_instruction(enum opcode instruction,
                                      const struct chunk *chunk,
                                      uint32_t offset) {

  uint32_t constant = 0;
  const enum opcode *code = chunk->code;

  switch (instruction) {
  case OPCODE_CONSTANT_24:
    constant |= code[++offset] & 0xff0000;
  case OPCODE_CONSTANT_16:
    constant |= code[++offset] & 0xff00;
  case OPCODE_CONSTANT_8:
    constant |= code[++offset] & 0xff;
  default:
    break;
  }

  fprintf(stderr, "%-16s %4u '", g_INSTRUCTION_NAME[instruction], constant);
  value_print(chunk->constants->values[constant]);
  fputs("'\n", stderr);
  return offset;
}

uint32_t _variable_instruction(enum opcode instruction,
                               const struct chunk *chunk, uint32_t offset) {
  uint32_t constant = 0;
  const enum opcode *code = chunk->code;

  switch (instruction) {
  case OPCODE_DEFINE_GLOBAL_24:
  case OPCODE_GET_GLOBAL_24:
    constant |= code[++offset] & 0xff0000;
  case OPCODE_DEFINE_GLOBAL_16:
  case OPCODE_GET_GLOBAL_16:
    constant |= code[++offset] & 0xff00;
  case OPCODE_DEFINE_GLOBAL_8:
  case OPCODE_GET_GLOBAL_8:
    constant |= code[++offset] & 0xff;
  default:
    break;
  }

  struct obj_string *string =
      VALUE_AS_STRING(chunk->constants->values[constant]);
  fprintf(stderr, "%-21s '%.*s'\n", g_INSTRUCTION_NAME[instruction],
          string->length,
          (string->is_owned) ? string->as.owned : string->as.ref);

  return offset;
}

uint32_t chunk_disassemble_instruction(const struct chunk *chunk,
                                       uint32_t offset) {
  uint32_t line = chunk_get_line(chunk, offset);
  fprintf(stderr, "%04u ", offset);

  if (offset > 0 && line == chunk_get_line(chunk, offset - 1)) {
    fputs("   | ", stderr);
  } else {
    fprintf(stderr, "%4u ", line);
  }

  enum opcode instruction = chunk->code[offset];
  const char *name = g_INSTRUCTION_NAME[instruction];

  switch (instruction) {
  case OPCODE_CONSTANT_8:
  case OPCODE_CONSTANT_16:
  case OPCODE_CONSTANT_24:
    return _constant_instruction(instruction, chunk, offset);
  case OPCODE_GET_GLOBAL_8:
  case OPCODE_GET_GLOBAL_16:
  case OPCODE_GET_GLOBAL_24:
  case OPCODE_DEFINE_GLOBAL_8:
  case OPCODE_DEFINE_GLOBAL_16:
  case OPCODE_DEFINE_GLOBAL_24:
    return _variable_instruction(instruction, chunk, offset);
  case OPCODE_NONE:
  case OPCODE_ADD:
  case OPCODE_SUB:
  case OPCODE_MUL:
  case OPCODE_DIV:
  case OPCODE_INT_DIV:
  case OPCODE_MOD:
  case OPCODE_NEGATE:
  case OPCODE_RETURN:
  case OPCODE_TRUE:
  case OPCODE_FALSE:
  case OPCODE_AND:
  case OPCODE_OR:
  case OPCODE_NOT:
  case OPCODE_EQUAL:
  case OPCODE_NOT_EQUAL:
  case OPCODE_LESS:
  case OPCODE_LESS_EQUAL:
  case OPCODE_GREATER:
  case OPCODE_GREATER_EQUAL:
  case OPCODE_CONCAT:
  case OPCODE_OUTPUT:
  case OPCODE_POP:
    _simple_instruction(name);
    return offset + 1;
  }
}

void chunk_disassemble(const struct chunk *chunk, const char *name) {
  fprintf(stderr, "== %s ==\n", name);
  for (uint32_t offset = 0; offset < chunk->count;) {
    offset = chunk_disassemble_instruction(chunk, offset);
  }
}
#endif // DEBUG_CHUNK
