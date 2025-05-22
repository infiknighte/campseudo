#ifndef CAMPSEUDO_VALUE_H
#define CAMPSEUDO_VALUE_H

#include "common.h"

#define VALUE_AS_OBJ(value) (value).as.obj
#define VALUE_AS_BOOL(value) (value).as.boolean
#define VALUE_AS_CHAR(value) (value).as.cha
#define VALUE_AS_REAL(value) (value).as.real
#define VALUE_AS_INTEGER(value) (value).as.integer
#define VALUE_AS_STRING(value) (obj_string_t)(value).as.obj

#define VALUE_FROM_OBJ(object)                                                 \
  (struct value) { VALUE_KIND_OBJ, .as.obj = (obj_t)object }
#define VALUE_FROM_BOOL(bool)                                                  \
  (struct value) { VALUE_KIND_BOOL, .as.boolean = bool }
#define VALUE_FROM_CHAR(char)                                                  \
  (struct value) { VALUE_KIND_CHAR, .as.cha = char }
#define VALUE_FROM_REAL(real_)                                                 \
  (struct value) { VALUE_KIND_REAL, .as.real = real_ }
#define VALUE_FROM_INTEGER(int)                                                \
  (struct value) { VALUE_KIND_INTEGER, .as.integer = int }

typedef struct obj *obj_t;
typedef struct obj_string *obj_string_t;

enum value_kind : uint8_t {
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
    obj_t obj;
  } as;
};

typedef struct value_array {
  uint32_t count, capacity;
  struct value values[];
} *value_array_t;

void value_array_new(value_array_t *array);
void value_array_free(value_array_t *array);
void value_array_write(value_array_t *array, struct value value);

bool value_is_equal(struct value a, struct value b);

#ifdef DEBUG_CHUNK
void value_print(struct value value);
#endif

#endif