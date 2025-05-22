#include "vm.h"
#include "chunk.h"
#include "obj.h"
#include "value.h"
#include <stdint.h>
#include <string.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <stdio.h>
#endif

void vm_init(struct vm *vm) {
  stack_new(&vm->stack);
  vm->objects = NULL;
}

void vm_free(struct vm *vm) {
  stack_free(&vm->stack);
  objects_free(vm->objects);
}

static void _concat(struct vm *vm) {
  obj_string_t b = VALUE_AS_STRING(stack_pop(vm->stack));
  obj_string_t a = VALUE_AS_STRING(stack_pop(vm->stack));

  obj_string_t c = obj_string_new(a->length + b->length);
  memcpy(c->as.owned, OBJ_AS_CSTRING(a), a->length);
  memcpy(c->as.owned + a->length, OBJ_AS_CSTRING(b), b->length);

  stack_put(&vm->stack, VALUE_FROM_OBJ(c));
}

static enum interpret_result _run(struct vm *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants->values[READ_BYTE()])
#define READ_CONSTANT_LONG()                                                   \
  (vm->chunk->constants                                                        \
       ->values[READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16)])
#define COMPARE_OP(op)                                                         \
  do {                                                                         \
    struct value b = stack_pop(vm->stack);                                     \
    struct value a = stack_pop(vm->stack);                                     \
    switch (a.kind) {                                                          \
    case VALUE_KIND_BOOL:                                                      \
      stack_put(&vm->stack,                                                    \
                VALUE_FROM_BOOL(VALUE_AS_BOOL(a) op VALUE_AS_BOOL(b)));        \
      break;                                                                   \
    case VALUE_KIND_CHAR:                                                      \
      stack_put(&vm->stack,                                                    \
                VALUE_FROM_BOOL(VALUE_AS_CHAR(a) op VALUE_AS_CHAR(b)));        \
      break;                                                                   \
    case VALUE_KIND_REAL:                                                      \
      stack_put(&vm->stack,                                                    \
                VALUE_FROM_BOOL(VALUE_AS_REAL(a) op VALUE_AS_REAL(b)));        \
      break;                                                                   \
    case VALUE_KIND_INTEGER:                                                   \
      stack_put(&vm->stack,                                                    \
                VALUE_FROM_BOOL(VALUE_AS_INTEGER(a) op VALUE_AS_INTEGER(b)));  \
      break;                                                                   \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
  } while (false)
#define BOOL_BINARY_OP(op)                                                     \
  do {                                                                         \
    bool b = stack_pop(vm->stack).as.boolean;                                  \
    struct value a = stack_pop(vm->stack);                                     \
    a.as.boolean = a.as.boolean op b;                                          \
    stack_put(&vm->stack, a);                                                  \
  } while (false)
#define NUMBER_BINARY_OP(op)                                                   \
  do {                                                                         \
    struct value b = stack_pop(vm->stack);                                     \
    struct value a = stack_pop(vm->stack);                                     \
    if (a.kind == VALUE_KIND_REAL) {                                           \
      a.as.real = a.as.real op b.as.real;                                      \
    } else {                                                                   \
      a.as.integer = a.as.integer op b.as.integer;                             \
    }                                                                          \
    stack_put(&vm->stack, a);                                                  \
  } while (false)
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    fputs("          ", stderr);
    for (struct value *slot = vm->stack->values; slot < vm->stack->top;
         slot++) {
      fputs("[ ", stderr);
      value_print(*slot);
      fputs(" ]", stderr);
    }
    fputc('\n', stderr);

    chunk_disassemble_instruction(vm->chunk, vm->ip - vm->chunk->code);
#endif
    enum opcode instruction;
    switch (instruction = READ_BYTE()) {
    case OPCODE_CONSTANT:
      stack_put(&vm->stack, READ_CONSTANT());
      break;
    case OPCODE_CONSTANT_LONG:
      stack_put(&vm->stack, READ_CONSTANT_LONG());
      break;
    case OPCODE_ADD:
      NUMBER_BINARY_OP(+);
      break;
    case OPCODE_SUB:
      NUMBER_BINARY_OP(-);
      break;
    case OPCODE_MUL:
      NUMBER_BINARY_OP(*);
      break;
    case OPCODE_DIV:
      NUMBER_BINARY_OP(/);
      break;
    case OPCODE_NEGATE: {
      struct value *top = &vm->stack->top[-1];
      switch (top->kind) {
      case VALUE_KIND_REAL:
        top->as.real = -top->as.real;
        break;
      case VALUE_KIND_INTEGER:
        return top->as.integer = -top->as.integer;
        break;
      default:
        return INTERPRET_RESULT_RUNTIME_ERROR;
      }
      break;
    }
    case OPCODE_RETURN:
      value_print(stack_pop(vm->stack));
      fputc('\n', stderr);
      return INTERPRET_RESULT_OK;
    case OPCODE_TRUE:
      stack_put(&vm->stack,
                (struct value){VALUE_KIND_BOOL, .as.boolean = true});
      break;
    case OPCODE_FALSE:
      stack_put(&vm->stack,
                (struct value){VALUE_KIND_BOOL, .as.boolean = false});
      break;
    case OPCODE_NOT:
      vm->stack->top[-1].as.boolean = !vm->stack->top[-1].as.boolean;
      break;
    case OPCODE_EQUAL: {
      struct value a = stack_pop(vm->stack);
      struct value b = stack_pop(vm->stack);
      stack_put(&vm->stack, (struct value){VALUE_KIND_BOOL,
                                           .as.boolean = value_is_equal(a, b)});
      break;
    }
    case OPCODE_NOT_EQUAL: {
      struct value a = stack_pop(vm->stack);
      struct value b = stack_pop(vm->stack);
      stack_put(&vm->stack, (struct value){VALUE_KIND_BOOL,
                                           .as.boolean = value_is_equal(a, b)});
      break;
    }
    case OPCODE_LESS:
      COMPARE_OP(<);
      break;
    case OPCODE_LESS_EQUAL:
      COMPARE_OP(<=);
      break;
    case OPCODE_GREATER:
      COMPARE_OP(>);
      break;
    case OPCODE_GREATER_EQUAL:
      COMPARE_OP(<=);
      break;
    case OPCODE_CONCAT:
      _concat(vm);
      break;
    }
  }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef COMPARE_OP
#undef BOOL_BINARY_OP
#undef NUMBER_BINARY_OP
}

enum interpret_result vm_interpret(struct vm *vm, const chunk_t chunk) {
  vm->chunk = chunk;
  vm->ip = chunk->code;

  enum interpret_result result = _run(vm);

  return result;
}
