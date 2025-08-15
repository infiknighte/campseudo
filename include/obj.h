#ifndef CAMPSEUDO_OBJ_H
#define CAMPSEUDO_OBJ_H

#include "table.h"
#include <stdbool.h>
#include <stdint.h>

#define AS_OBJ(object) ((struct obj *)object)
#define OBJ_AS_STRING(obj) ((struct obj_string *)obj)
#define OBJ_AS_CSTRING(obj)                                                    \
  (OBJ_AS_STRING(obj)->is_owned ? OBJ_AS_STRING(obj)->as.owned                 \
                                : OBJ_AS_STRING(obj)->as.ref)

enum obj_kind { OBJ_KIND_STRING };

struct obj {
  enum obj_kind kind;
  struct obj *next;
};

struct obj_string {
  struct obj obj;
  uint32_t length;
  bool is_owned;
  uint32_t hash;
  union {
    const uint8_t *ref;
    uint8_t owned[];
  } as;
};

void obj_eprint(const struct obj *obj);

struct obj_string *obj_string_copy(struct obj **objects, struct table **strings,
                                   const uint8_t *chars, uint32_t length);
struct obj_string *obj_string_ref(struct obj **objects, struct table **strings,
                                  const uint8_t *chars, uint32_t length);
void objects_free(struct obj *objects);

#endif