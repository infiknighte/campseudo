#ifndef CAMPSEUDO_VM_H
#define CAMPSEUDO_VM_H

#include "chunk.h"
#include "stack.h"

struct vm {
  uint8_t *ip;
  obj_t objects;
  chunk_t chunk;
  stack_t stack;
};

enum interpret_result {
  INTERPRET_RESULT_OK,
  INTERPRET_RESULT_COMPILE_ERROR,
  INTERPRET_RESULT_RUNTIME_ERROR,
};

void vm_init(struct vm *vm);
void vm_free(struct vm *vm);
enum interpret_result vm_interpret(struct vm *vm, const chunk_t chunk);
void obj_free(obj_t obj);
void objects_free(obj_t obj);

#endif