#ifndef CAMPSEUDO_VALUE_H
#define CAMPSEUDO_VALUE_H

#include "common.h"
#include "memory.h"

#define VALUE_AS_OBJ(value) (value).as.obj
#define VALUE_AS_BOOL(value) (value).as.boolean
#define VALUE_AS_CHAR(value) (value).as.cha
#define VALUE_AS_REAL(value) (value).as.real
#define VALUE_AS_INTEGER(value) (value).as.integer
#define VALUE_AS_STRING(value) (struct obj_string *)(value).as.obj
#define VALUE_AS_NUMBER(value)                                                 \
  ((value).kind == VALUE_KIND_INTEGER ? (value).as.integer : (value).as.real)

#define VALUE_FROM_OBJ(object)                                                 \
  (struct value) { VALUE_KIND_OBJ, .as.obj = (struct obj *)object }
#define VALUE_FROM_BOOL(bool)                                                  \
  (struct value) { VALUE_KIND_BOOL, .as.boolean = bool }
#define VALUE_FROM_CHAR(char)                                                  \
  (struct value) { VALUE_KIND_CHAR, .as.cha = char }
#define VALUE_FROM_REAL(real_)                                                 \
  (struct value) { VALUE_KIND_REAL, .as.real = real_ }
#define VALUE_FROM_INTEGER(int)                                                \
  (struct value) { VALUE_KIND_INTEGER, .as.integer = int }
#define VALUE_NONE()                                                           \
  (struct value) { VALUE_KIND_NONE }

struct obj;
struct obj_string;

enum value_kind : uint8_t {
  VALUE_KIND_NONE,
  VALUE_KIND_BOOL,
  VALUE_KIND_CHAR,
  VALUE_KIND_REAL,
  VALUE_KIND_INTEGER,
  VALUE_KIND_OBJ,
};

struct value {
  enum value_kind kind;
  union {
    bool boolean;
    uint8_t cha;
    double real;
    int64_t integer;
    struct obj *obj;
  } as;
};

struct value_array {
  uint32_t count, capacity;
  struct value values[];
};

struct value_array *value_array_new(void);
void value_array_write(struct value_array **array, struct value value);

bool value_is_equal(struct value a, struct value b);

static inline void value_array_free(struct value_array *array) {
  MEM_FREE(array,
           sizeof(struct value_array) + array->capacity * sizeof(struct value));
}

#ifdef DEBUG_CHUNK
void value_print(struct value value);
#endif

#endif