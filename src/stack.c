#include "stack.h"
#include "memory.h"
#include <stdint.h>
#include <stdlib.h>

#define CAPACITY_INIT 8U
#define CAPACITY_MULT 2U

struct stack *stack_new(void) {
  struct stack *stack =
      MEM_ALLOC(sizeof(struct stack) + CAPACITY_INIT * sizeof(struct value));
  stack->capacity = CAPACITY_INIT;
  stack->top = stack->values;
  return stack;
}

void stack_put(struct stack **stack, struct value value) {
  struct stack *self = *stack;
  if (self->top - self->values >= self->capacity) {
    size_t count = self->top - self->values;
    size_t new_capacity = self->capacity * CAPACITY_MULT;

    *stack = self = reallocate(
        self, sizeof(struct stack) + self->capacity * sizeof(struct value),
        sizeof(struct stack) + new_capacity * sizeof(struct value));

    self->capacity = new_capacity;
    self->top = self->values + count;
  }

  *(self->top++) = value;
}
