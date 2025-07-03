#include "vm.h"
#include "chunk.h"
#include "obj.h"
#include "stack.h"
#include "table.h"
#include "value.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <stdio.h>
#endif

void vm_init(struct vm *vm) {
  vm->objects = NULL;
  vm->stack = stack_new();
  vm->globals = table_new();
  vm->strings = table_new();
}

void vm_free(const struct vm *vm) {
  stack_free(vm->stack);
  table_free(vm->globals);
  table_free(vm->strings);
  objects_free(vm->objects);
}

static void _concat(struct vm *vm) {
  const struct obj_string *b = VALUE_AS_STRING(stack_pop(vm->stack));
  const struct obj_string *a = VALUE_AS_STRING(stack_pop(vm->stack));

  uint32_t length = a->length + b->length;
  uint8_t c[length];
  memcpy(c, OBJ_AS_CSTRING(a), a->length);
  memcpy(c + a->length, OBJ_AS_CSTRING(b), b->length);

  stack_put(&vm->stack, VALUE_FROM_OBJ(obj_string_copy(
                            &vm->objects, &vm->strings, c, length)));
}

static enum interpret_result _run(struct vm *vm) {
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT_8(index) (vm->chunk->constants->values[READ_BYTE()])
#define READ_CONSTANT_16(index)                                                \
  (vm->chunk->constants->values[READ_BYTE() | READ_BYTE() << 8])
#define READ_CONSTANT_24(index)                                                \
  (vm->chunk->constants                                                        \
       ->values[READ_BYTE() | READ_BYTE() << 8 | READ_BYTE() < 16])
#define READ_CONSTANT_32(index)                                                \
  (vm->chunk->constants->values[READ_BYTE() | READ_BYTE() << 8 |               \
                                READ_BYTE() << 16 | READ_BYTE() << 24])
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
    bool b = VALUE_AS_BOOL(stack_pop(vm->stack));                              \
    struct value a = stack_pop(vm->stack);                                     \
    a.as.boolean = a.as.boolean op b;                                          \
    stack_put(&vm->stack, a);                                                  \
  } while (false)
#define NUMBER_BINARY_OP(op)                                                   \
  do {                                                                         \
    struct value b = stack_pop(vm->stack);                                     \
    struct value a = stack_pop(vm->stack);                                     \
    if (a.kind == VALUE_KIND_REAL) {                                           \
      a.as.real = a.as.real op VALUE_AS_NUMBER(b);                             \
    } else {                                                                   \
      if (b.kind == VALUE_KIND_REAL) {                                         \
        a.as.real = a.as.integer op b.as.real;                                 \
        a.kind = VALUE_KIND_REAL;                                              \
      } else {                                                                 \
        a.as.integer = a.as.integer op b.as.integer;                           \
      }                                                                        \
    }                                                                          \
    stack_put(&vm->stack, a);                                                  \
  } while (false)
#define DEFINE_VARIABLE(byte)                                                  \
  do {                                                                         \
    struct obj_string *name = VALUE_AS_STRING(READ_CONSTANT_##byte());         \
    struct value value;                                                        \
    if (table_member(vm->globals, name, &value)) {                             \
      return INTERPRET_RESULT_RUNTIME_ERROR;                                   \
    }                                                                          \
    table_insert(&vm->globals, name, *stack_peek(vm->stack, 0));               \
    stack_pop(vm->stack);                                                      \
  } while (false)
#define GET_VARIABLE(byte)                                                     \
  do {                                                                         \
    struct obj_string *name = VALUE_AS_STRING(READ_CONSTANT_##byte());         \
    struct value value;                                                        \
    if (!table_member(vm->globals, name, &value)) {                            \
      return INTERPRET_RESULT_RUNTIME_ERROR;                                   \
    }                                                                          \
    stack_put(&vm->stack, value);                                              \
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
    case OPCODE_NONE:
      stack_put(&vm->stack, VALUE_NONE());
      break;
    case OPCODE_CONSTANT_8:
      stack_put(&vm->stack, READ_CONSTANT_8());
      break;
    case OPCODE_CONSTANT_16:
      stack_put(&vm->stack, READ_CONSTANT_16());
      break;
    case OPCODE_CONSTANT_24:
      stack_put(&vm->stack, READ_CONSTANT_24());
      break;
    case OPCODE_GET_GLOBAL_8:
      GET_VARIABLE(8);
      break;
    case OPCODE_GET_GLOBAL_16:
      GET_VARIABLE(16);
      break;
    case OPCODE_GET_GLOBAL_24:
      GET_VARIABLE(24);
      break;
    case OPCODE_DEFINE_GLOBAL_8:
      DEFINE_VARIABLE(8);
      break;
    case OPCODE_DEFINE_GLOBAL_16:
      DEFINE_VARIABLE(16);
      break;
    case OPCODE_DEFINE_GLOBAL_24:
      DEFINE_VARIABLE(24);
      break;
    case OPCODE_POP:
      stack_pop(vm->stack);
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
    case OPCODE_DIV: {
      struct value b = stack_pop(vm->stack);
      struct value a = stack_pop(vm->stack);
      a.as.real = (double)VALUE_AS_NUMBER(a) / (double)VALUE_AS_NUMBER(b);
      a.kind = VALUE_KIND_REAL;
      stack_put(&vm->stack, a);
      break;
    }
    case OPCODE_INT_DIV: {
      struct value b = stack_pop(vm->stack);
      struct value a = stack_pop(vm->stack);
      a.as.integer = VALUE_AS_NUMBER(a) / VALUE_AS_NUMBER(b);
      a.kind = VALUE_KIND_INTEGER;
      stack_put(&vm->stack, a);
      break;
    }
    case OPCODE_MOD: {
      struct value b = stack_pop(vm->stack);
      struct value a = stack_pop(vm->stack);
      if (a.kind == VALUE_KIND_INTEGER && b.kind == VALUE_KIND_INTEGER) {
        a.as.integer = VALUE_AS_INTEGER(a) % VALUE_AS_INTEGER(b);
        stack_put(&vm->stack, a);
      } else {
        a.as.real = fmod(VALUE_AS_NUMBER(a), VALUE_AS_NUMBER(b));
        a.kind = VALUE_KIND_REAL;
        stack_put(&vm->stack, a);
      }
      break;
    }
    case OPCODE_NEGATE: {
      struct value *top = stack_peek(vm->stack, 0);
      switch (top->kind) {
      case VALUE_KIND_REAL:
        top->as.real = -top->as.real;
        break;
      case VALUE_KIND_INTEGER:
        top->as.integer = -top->as.integer;
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
    case OPCODE_AND:
      BOOL_BINARY_OP(&&);
      break;
    case OPCODE_OR:
      BOOL_BINARY_OP(||);
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
      stack_put(
          &vm->stack,
          (struct value){VALUE_KIND_BOOL, .as.boolean = !value_is_equal(a, b)});
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
      COMPARE_OP(>=);
      break;
    case OPCODE_CONCAT:
      _concat(vm);
      break;
    case OPCODE_OUTPUT:
      value_print(stack_pop(vm->stack));
      putchar('\n');
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

enum interpret_result vm_interpret(struct vm *vm, const struct chunk *chunk) {
  vm->chunk = chunk;
  vm->ip = chunk->code;

  enum interpret_result result = _run(vm);

  return result;
}