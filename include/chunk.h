#ifndef COMPSEUDO_CHUNK_H
#define COMPSEUDO_CHUNK_H

#include "ast.h"
#include "common.h"
#include "value.h"
#include <stdint.h>

enum opcode : uint8_t {
  OPCODE_CONSTANT,
  OPCODE_CONSTANT_LONG,
  OPCODE_TRUE,
  OPCODE_FALSE,
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_DIV,
  OPCODE_CONCAT,
  OPCODE_NEGATE,
  OPCODE_NOT,
  OPCODE_EQUAL,
  OPCODE_NOT_EQUAL,
  OPCODE_LESS,
  OPCODE_LESS_EQUAL,
  OPCODE_GREATER,
  OPCODE_GREATER_EQUAL,
  OPCODE_RETURN,
};

typedef struct line_array {
  uint32_t count, capacity;
  uint32_t lines[];
} *line_array_t;

typedef struct chunk {
  uint32_t count, capacity;
  value_array_t constants;
  line_array_t lines;
  enum opcode code[];
} *chunk_t;

void chunk_new(chunk_t *chunk);
void chunk_free(chunk_t *chunk);
void chunk_write(chunk_t *chunk, enum opcode byte, uint32_t line);
uint32_t chunk_add_constant(chunk_t chunk, struct value value);
uint32_t chunk_get_line(chunk_t chunk, uint32_t index);
uint32_t chunk_write_constant(chunk_t *chunk, struct value value,
                              uint32_t line);
void chunk_write_from_ast(chunk_t *chunk, struct ast *ast);

#ifdef DEBUG_CHUNK
void chunk_disassemble(chunk_t chunk, const char *name);
uint32_t chunk_disassemble_instruction(chunk_t chunk, uint32_t offset);
#endif

#endif