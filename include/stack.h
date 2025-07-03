#ifndef COMPSEUDO_STACK_H
#define COMPSEUDO_STACK_H

#include "memory.h"
#include "value.h"

struct stack {
  struct value *top;
  uint32_t capacity;
  struct value values[];
};

struct stack *stack_new(void);
void stack_put(struct stack **stack, struct value value);

static inline struct value stack_pop(struct stack *stack) {
  return *--stack->top;
}
static inline struct value *stack_peek(const struct stack *stack, uint32_t at) {
  return stack->top - (at + 1);
}

static inline void stack_reset(struct stack *stack) {
  stack->top = stack->values;
}

static inline void stack_free(struct stack *stack) {
  MEM_FREE(stack,
           sizeof(struct stack) + stack->capacity * sizeof(struct value));
}

#endif