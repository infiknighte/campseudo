#include "value.h"
#include "memory.h"
#include "obj.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACITY_INIT 8U
#define CAPACITY_MULT 2U

void value_array_new(value_array_t *array) {
  *array = reallocate(
      NULL, 0, sizeof(value_array_t) + CAPACITY_INIT * sizeof(struct value));
  (*array)->capacity = CAPACITY_INIT;
  (*array)->count = 0;
}

void value_array_free(value_array_t *array) {
  reallocate(*array,
             sizeof(struct value_array) +
                 (*array)->capacity * sizeof(struct value),
             0);
  *array = NULL;
}

void value_array_write(value_array_t *array, struct value value) {
  if ((*array)->capacity < (*array)->count + 1) {
    uint32_t new_capcity = (*array)->capacity * CAPACITY_MULT;
    *array = reallocate(
        *array,
        sizeof(struct value_array) + (*array)->count * sizeof(struct value),
        sizeof(struct value_array) + new_capcity * sizeof(struct value));
    (*array)->capacity = new_capcity;
  }
  (*array)->values[(*array)->count++] = value;
}

bool value_is_equal(struct value a, struct value b) {
  if (a.kind != b.kind) {
    return false;
  }
  switch (a.kind) {
  case VALUE_KIND_BOOL:
    return VALUE_AS_BOOL(a) == VALUE_AS_BOOL(b);
  case VALUE_KIND_CHAR:
    return VALUE_AS_CHAR(a) == VALUE_AS_BOOL(b);
  case VALUE_KIND_REAL:
    return VALUE_AS_REAL(a) == VALUE_AS_BOOL(b);
  case VALUE_KIND_INTEGER:
    return VALUE_AS_INTEGER(a) == VALUE_AS_BOOL(b);
  case VALUE_KIND_OBJ: {
    obj_string_t a_str = VALUE_AS_STRING(a);
    obj_string_t b_str = VALUE_AS_STRING(b);
    return a_str->length == b_str->length &&
           !memcmp(OBJ_AS_CSTRING(a_str), OBJ_AS_CSTRING(a_str), a_str->length);
  }
  }
}

#ifdef DEBUG_CHUNK
#include <stdio.h>

void value_print(struct value value) {
  switch (value.kind) {
  case VALUE_KIND_BOOL:
    fputs(value.as.boolean ? "TRUE" : "FALSE", stderr);
    break;
  case VALUE_KIND_CHAR:
    fprintf(stderr, "'%c'", VALUE_AS_CHAR(value));
    break;
  case VALUE_KIND_REAL:
    fprintf(stderr, "%g", VALUE_AS_REAL(value));
    break;
  case VALUE_KIND_INTEGER:
    fprintf(stderr, "%lli", VALUE_AS_INTEGER(value));
    break;
  case VALUE_KIND_OBJ:
    obj_print(VALUE_AS_OBJ(value));
    break;
  }
}

#endif