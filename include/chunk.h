#ifndef COMPSEUDO_CHUNK_H
#define COMPSEUDO_CHUNK_H

#include "ast.h"
#include "common.h"
#include "table.h"
#include "value.h"
#include <stdint.h>

enum opcode : uint8_t {
  OPCODE_NONE,
  OPCODE_CONSTANT_8,
  OPCODE_CONSTANT_16,
  OPCODE_CONSTANT_24,
  OPCODE_DEF_GLOBAL_8,
  OPCODE_DEF_GLOBAL_16,
  OPCODE_DEF_GLOBAL_24,
  OPCODE_GET_GLOBAL_8,
  OPCODE_GET_GLOBAL_16,
  OPCODE_GET_GLOBAL_24,
  OPCODE_POP,
  OPCODE_TRUE,
  OPCODE_FALSE,
  OPCODE_AND,
  OPCODE_OR,
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_DIV,
  OPCODE_INT_DIV,
  OPCODE_MOD,
  OPCODE_CONCAT,
  OPCODE_NEGATE,
  OPCODE_NOT,
  OPCODE_EQUAL,
  OPCODE_NOT_EQUAL,
  OPCODE_LESS,
  OPCODE_LESS_EQUAL,
  OPCODE_GREATER,
  OPCODE_GREATER_EQUAL,
  OPCODE_OUTPUT,
  OPCODE_RETURN,
};

struct line_array {
  uint32_t count, capacity;
  uint32_t lines[];
};

struct chunk {
  uint64_t count, capacity;
  struct value_array *constants;
  struct line_array *lines;
  enum opcode code[];
};

struct chunk *chunk_new(void);
void chunk_free(struct chunk *chunk);
void chunk_write(struct chunk **chunk, enum opcode byte, uint32_t line);
uint32_t chunk_add_constant(struct chunk *chunk, struct value value);
uint32_t chunk_get_line(const struct chunk *chunk, uint32_t index);
uint32_t chunk_write_constant(struct chunk **chunk, struct value value,
                              uint32_t line);
void chunk_write_ast(struct chunk **chunk, struct ast *ast,
                     struct obj **objects, struct table **strings);

static inline void chunk_reset(struct chunk *chunk) {
  chunk->count = 0;
  chunk->lines->count = 0;
}

#ifdef DEBUG_CHUNK
void chunk_disassemble(const struct chunk *chunk, const char *name);
uint32_t chunk_disassemble_instruction(const struct chunk *chunk,
                                       uint32_t offset);
#endif

#endif