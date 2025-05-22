#ifndef COMPSEUDO_STACK_H
#define COMPSEUDO_STACK_H

#include "value.h"

typedef struct stack {
  struct value *top;
  uint32_t capacity;
  struct value values[];
} *stack_t;

void stack_new(stack_t *stack);
void stack_put(stack_t *stack, struct value value);
struct value stack_pop(stack_t stack);
void stack_reset(stack_t stack);
void stack_free(stack_t *stack);

#endif