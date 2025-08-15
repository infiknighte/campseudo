#ifndef CAMPSEUDO_VM_H
#define CAMPSEUDO_VM_H

#include "chunk.h"
#include "stack.h"
#include "table.h"

struct vm {
  const enum opcode *ip;
  const struct chunk *chunk;
  struct obj *objects;
  struct stack *stack;
  struct table *globals;
  struct table *strings;
};

enum interpret_result {
  INTERPRET_RESULT_OK,
  INTERPRET_RESULT_COMPILE_ERROR,
  INTERPRET_RESULT_RUNTIME_ERROR,
};

void vm_init(struct vm *vm);
void vm_free(const struct vm *vm);
enum interpret_result vm_interpret(struct vm *vm, const struct chunk *chunk);
void obj_free(struct obj *obj);

static inline void vm_reset(struct vm *vm) {
  vm->ip = vm->chunk->code;
  stack_reset(vm->stack);
}

static inline void vm_stack_reset(const struct vm *vm) {
  stack_reset(vm->stack);
}

#endif