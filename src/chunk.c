#include "chunk.h"
#include "memory.h"
#include "obj.h"
#include "value.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CAPACITY_INIT 8U
#define CAPACITY_MULT 2U

static void _line_write(line_array_t *array, uint32_t line) {
  uint32_t count = (*array)->count;
  if (count > 1 && line == (*array)->lines[count - 2]) {
    (*array)->lines[count - 1]++;
    return;
  }

  if ((*array)->capacity < count + 2) {
    uint32_t new_capacity = (*array)->capacity * CAPACITY_MULT;
    void *temp = reallocate(
        *array,
        sizeof(struct line_array) + (*array)->capacity * sizeof(uint32_t),
        sizeof(struct line_array) + new_capacity * sizeof(uint32_t));
    if (!temp) {
      free(*array);
      exit(1);
    }
    *array = temp;
    (*array)->capacity = new_capacity;
  }

  (*array)->lines[count] = line;
  (*array)->lines[count + 1] = 1;
  (*array)->count += 2;
}

void chunk_new(chunk_t *chunk) {
  *chunk = reallocate(NULL, 0, sizeof(struct chunk) + CAPACITY_INIT);
  (*chunk)->count = 0;
  value_array_new(&(*chunk)->constants);
  (*chunk)->capacity = CAPACITY_INIT;
  (*chunk)->lines = reallocate(
      NULL, 0, sizeof(struct line_array) + sizeof(uint32_t) * CAPACITY_INIT);
  (*chunk)->lines->capacity = CAPACITY_INIT;
  (*chunk)->lines->count = 0;
}

void chunk_free(chunk_t *chunk) {
  value_array_free(&(*chunk)->constants);
  reallocate(
      (*chunk)->lines,
      sizeof(line_array_t) + sizeof(uint32_t) * (*chunk)->lines->capacity, 0);
  reallocate(*chunk, sizeof(struct chunk) + CAPACITY_INIT, 0);
  *chunk = NULL;
}

void chunk_write(chunk_t *chunk, enum opcode byte, uint32_t line) {
  if ((*chunk)->capacity < (*chunk)->count + 1) {
    uint32_t new_capcity = (*chunk)->capacity * CAPACITY_MULT;
    *chunk = reallocate(*chunk, sizeof(struct chunk) + (*chunk)->capacity,
                        sizeof(struct chunk) + new_capcity);
    (*chunk)->capacity = new_capcity;
  }
  (*chunk)->code[(*chunk)->count++] = byte;
  _line_write(&(*chunk)->lines, line);
}

uint32_t chunk_add_constant(chunk_t chunk, struct value value) {
  value_array_write(&chunk->constants, value);
  return chunk->constants->count - 1;
}

uint32_t chunk_write_constant(chunk_t *chunk, struct value value,
                              uint32_t line) {
  uint32_t constant = chunk_add_constant(*chunk, value);

  chunk_write(chunk, constant & 0xFFU, line);
  chunk_write(chunk, constant & 0xFF00U, line);
  chunk_write(chunk, constant & 0xFF0000U, line);

  return constant;
}

uint32_t chunk_get_line(chunk_t chunk, uint32_t index) {
  line_array_t array = chunk->lines;
  uint32_t n = 0;
  for (uint32_t i = 1; i < array->count; i += 2) {
    if (index < (n += array->lines[i])) {
      return array->lines[i - 1];
    }
  }
  return 0;
}

void chunk_write_from_ast(chunk_t *chunk, struct ast *ast) {
#define WRITE_VALUE(value)                                                     \
  do {                                                                         \
    if (255 > (*chunk)->count) {                                               \
      chunk_write(chunk, OPCODE_CONSTANT, ast->line);                          \
      chunk_write(chunk, chunk_add_constant(*chunk, value), ast->line);        \
    } else {                                                                   \
      chunk_write(chunk, OPCODE_CONSTANT_LONG, ast->line);                     \
      chunk_write_constant(chunk, value, ast->line);                           \
    }                                                                          \
  } while (false)

#define WRITE_UNARY(opcode)                                                    \
  chunk_write_from_ast(chunk, ast->as.expr);                                   \
  chunk_write(chunk, opcode, ast->line)

#define WRITE_BINARY(opcode)                                                   \
  chunk_write_from_ast(chunk, ast->as.binary.lhs);                             \
  chunk_write_from_ast(chunk, ast->as.binary.rhs);                             \
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
    WRITE_VALUE(VALUE_FROM_OBJ(
        obj_string_ref(ast->as.string.chars + 1, ast->as.string.length - 2)));
    break;
  case NODE_KIND_NOT:
    WRITE_UNARY(OPCODE_NOT);
    break;
  case NODE_KIND_NEGATE:
    WRITE_UNARY(OPCODE_NEGATE);
    break;
  case NODE_KIND_POINTER:
    // WRITE_UNARY(OPCODE_POINTER);
    break;
  case NODE_KIND_GROUP:
    chunk_write_from_ast(chunk, ast->as.expr);
    break;
  case NODE_KIND_ADD:
    WRITE_BINARY(OPCODE_ADD);
  case NODE_KIND_SUB:
    break;
    WRITE_BINARY(OPCODE_SUB);
  case NODE_KIND_MUL:
    WRITE_BINARY(OPCODE_MUL);
    break;
  case NODE_KIND_DIV:
    WRITE_BINARY(OPCODE_DIV);
    break;
  case NODE_KIND_INT_DIV:
    WRITE_BINARY(OPCODE_DIV);
    break;
  case NODE_KIND_MOD:
    // WRITE_BINARY(OPCODE_MOD);
    break;
  case NODE_KIND_AND:
    // WRITE_BINARY(OPCODE_AND);
    break;
  case NODE_KIND_OR:
    // WRITE_BINARY(OPCODE_OR);
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
  }
#undef WRITE_CONSTANT
#undef WRITE_UNARY
#undef WRITE_BINARY
}

#ifdef DEBUG_CHUNK
#include "stdio.h"

static uint32_t simple_instruction(const char *name, uint32_t offset) {
  fprintf(stderr, "%s\n", name);
  return offset + 1;
}

static uint32_t constant_instruction(const char *name, chunk_t chunk,
                                     uint32_t offset) {
  uint8_t constant = chunk->code[offset + 1];
  fprintf(stderr, "%-16s %4d '", name, constant);
  value_print(chunk->constants->values[constant]);
  fputs("'\n", stderr);
  return offset + 2;
}

static uint32_t constant_instruction_long(const char *name, chunk_t chunk,
                                          uint32_t offset) {
  uint32_t constant = chunk->code[offset + 1] | chunk->code[offset + 2] |
                      chunk->code[offset + 3];
  fprintf(stderr, "%-16s %4d '", name, constant);
  value_print(chunk->constants->values[constant]);
  fputs("'\n", stderr);
  return offset + 4;
}

uint32_t chunk_disassemble_instruction(chunk_t chunk, uint32_t offset) {
  uint32_t line = chunk_get_line(chunk, offset);

  fprintf(stderr, "%04d ", offset);

  if (offset > 0 && line == chunk_get_line(chunk, offset - 1)) {
    fputs("   | ", stderr);
  } else {
    fprintf(stderr, "%4d ", line);
  }

  enum opcode instruction = chunk->code[offset];
  switch (instruction) {
  case OPCODE_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OPCODE_CONSTANT_LONG:
    return constant_instruction_long("OP_CONSTANT_LONG", chunk, offset);
  case OPCODE_ADD:
    return simple_instruction("OP_ADD", offset);
  case OPCODE_SUB:
    return simple_instruction("OP_SUB", offset);
  case OPCODE_MUL:
    return simple_instruction("OP_MUL", offset);
  case OPCODE_DIV:
    return simple_instruction("OP_DIV", offset);
  case OPCODE_NEGATE:
    return simple_instruction("OP_NEGATE", offset);
  case OPCODE_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OPCODE_TRUE:
    return simple_instruction("OP_TRUE", offset);
  case OPCODE_FALSE:
    return simple_instruction("OP_FALSE", offset);
  case OPCODE_NOT:
    return simple_instruction("OP_NOT", offset);
  case OPCODE_EQUAL:
    return simple_instruction("OP_EQUAL", offset);
  case OPCODE_NOT_EQUAL:
    return simple_instruction("OP_NOT_EQUAL", offset);
  case OPCODE_LESS:
    return simple_instruction("OP_LESS", offset);
  case OPCODE_LESS_EQUAL:
    return simple_instruction("OP_LESS_EQUAL", offset);
  case OPCODE_GREATER:
    return simple_instruction("OP_GREATER", offset);
  case OPCODE_GREATER_EQUAL:
    return simple_instruction("OP_GREATER_EQUAL", offset);
  case OPCODE_CONCAT:
    return simple_instruction("OP_CONCAT", offset);
  default:
    fprintf(stderr, "Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

void chunk_disassemble(chunk_t chunk, const char *name) {
  fprintf(stderr, "== %s ==\n", name);
  for (uint32_t offset = 0; offset < chunk->count;) {
    offset = chunk_disassemble_instruction(chunk, offset);
  }
}
#endif // DEBUG_CHUNK
