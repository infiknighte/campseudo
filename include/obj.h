#ifndef CAMPSEUDO_OBJ_H
#define CAMPSEUDO_OBJ_H

#include "table.h"
#include <stdbool.h>
#include <stdint.h>

#define AS_OBJ(obj) ((obj_t)obj)
#define OBJ_AS_STRING(obj) ((obj_string_t)obj)
#define OBJ_AS_CSTRING(obj)                                                    \
  (OBJ_AS_STRING(obj)->is_owned ? OBJ_AS_STRING(obj)->as.owned                 \
                                : OBJ_AS_STRING(obj)->as.ref)

enum obj_kind { OBJ_KIND_STRING };

typedef struct obj {
  enum obj_kind kind;
  struct obj *next;
} *obj_t;

typedef struct obj_string {
  struct obj obj;
  uint32_t length;
  bool is_owned;
  uint32_t hash;
  union {
    const char *ref;
    char owned[];
  } as;
} *obj_string_t;

void obj_print(const obj_t obj);

obj_string_t obj_string_copy(obj_t *objects, table_t *strings,
                             const char *chars, uint32_t length);
obj_string_t obj_string_ref(obj_t *objects, table_t *strings, const char *chars,
                            uint32_t length);
void objects_free(obj_t *objects);

#endif