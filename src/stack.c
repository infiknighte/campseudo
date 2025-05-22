#include "stack.h"
#include "memory.h"
#include <stdlib.h>

#define CAPACITY_INIT 8U
#define CAPACITY_MULT 2U

void stack_new(stack_t *stack) {
  *stack = reallocate(
      NULL, 0, sizeof(struct stack) + CAPACITY_INIT * sizeof(struct value));
  (*stack)->capacity = CAPACITY_INIT;
  (*stack)->top = (*stack)->values;
}

void stack_put(stack_t *stack, struct value value) {
  if ((*stack)->top - (*stack)->values >= (*stack)->capacity) {
    size_t count = (*stack)->top - (*stack)->values;
    size_t new_capacity = (*stack)->capacity * CAPACITY_MULT;

    *stack = reallocate(
        *stack,
        sizeof(struct stack) + (*stack)->capacity * sizeof(struct value),
        sizeof(struct stack) + new_capacity * sizeof(struct value));

    (*stack)->capacity = new_capacity;
    (*stack)->top = (*stack)->values + count;
  }

  *((*stack)->top) = value;
  (*stack)->top++;
}

struct value stack_pop(stack_t stack) { return *--stack->top; }

void stack_reset(stack_t stack) { stack->top = stack->values; }

void stack_free(stack_t *stack) {
  reallocate(*stack,
             sizeof(struct stack) + (*stack)->capacity * sizeof(struct value),
             0);
  *stack = NULL;
}