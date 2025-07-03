#include "obj.h"
#include "common.h"
#include "memory.h"
#include "table.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_OBJ(type, kind) (type *)_allocate_obj(kind, sizeof(type))

static struct obj *_allocate_obj(struct obj **objects, enum obj_kind kind,
                                 size_t size) {
  struct obj *obj = reallocate(NULL, 0, size);
  obj->kind = kind;
  obj->next = *objects;
  *objects = obj;
  return obj;
}

static struct obj_string *_allocate_obj_string(struct obj **objects,
                                               struct table **strings,
                                               bool is_owned, uint32_t length,
                                               uint32_t hash) {
  struct obj_string *string =
      reallocate(NULL, 0,
                 (is_owned) ? sizeof(struct obj_string)
                            : sizeof(struct obj_string) + length + 1);
  string->is_owned = is_owned;
  string->length = length;
  string->obj.kind = OBJ_KIND_STRING;
  string->hash = hash;

  string->obj.next = *objects;
  *objects = AS_OBJ(string);

  table_insert(strings, string, TABLE_NIL);

  return string;
}

struct obj_string *obj_string_copy(struct obj **objects, struct table **strings,
                                   const uint8_t *chars, uint32_t length) {
  uint32_t hash = table_hash(chars, length);

  struct obj_string *interned =
      table_find_string(*strings, chars, length, hash);
  if (interned) {
    return interned;
  }

  struct obj_string *string =
      _allocate_obj_string(objects, strings, true, length, hash);
  memcpy(string->as.owned, chars, length);
  string->as.owned[length] = 0;

  return string;
}

struct obj_string *obj_string_ref(struct obj **objects, struct table **strings,
                                  const uint8_t *chars, uint32_t length) {
  uint32_t hash = table_hash(chars, length);
  struct obj_string *interned =
      table_find_string(*strings, chars, length, hash);
  if (interned) {
    return interned;
  }

  struct obj_string *string =
      _allocate_obj_string(objects, strings, false, length, hash);
  string->as.ref = chars;

  return string;
}

static void obj_free(struct obj *obj) {
  switch (obj->kind) {
  case OBJ_KIND_STRING: {
    struct obj_string *string = OBJ_AS_STRING(obj);
    if (string->is_owned) {
      MEM_FREE(string, sizeof(struct obj_string) + string->length + 1);
    } else {
      MEM_FREE(string, sizeof(struct obj_string));
    }
    break;
  }
  }
}

void objects_free(struct obj *objects) {
  while (objects) {
    struct obj *next = objects->next;
    obj_free(objects);
    objects = next;
  }
}

#ifdef DEBUG_OBJ
#include <stdio.h>

void obj_print(const struct obj *obj) {
  switch (obj->kind) {
  case OBJ_KIND_STRING:
    if (OBJ_AS_STRING(obj)->is_owned) {
      fprintf(stderr, "\"%s\"", OBJ_AS_STRING(obj)->as.owned);
    } else {
      fprintf(stderr, "&\"%.*s\"", OBJ_AS_STRING(obj)->length,
              OBJ_AS_STRING(obj)->as.ref);
    }
    break;
  }
}
#endif