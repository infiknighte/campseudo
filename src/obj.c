#include "obj.h"
#include "common.h"
#include "memory.h"
#include "table.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_OBJ(type, kind) (type *)_allocate_obj(kind, sizeof(type))

static obj_t _allocate_obj(obj_t *objects, enum obj_kind kind, size_t size) {
  obj_t obj = reallocate(NULL, 0, size);
  obj->kind = kind;
  obj->next = *objects;
  *objects = obj;
  return obj;
}

static obj_string_t _allocate_obj_string(obj_t *objects, table_t *strings,
                                         const char *chars, uint32_t length,
                                         uint32_t hash) {
  obj_string_t string =
      reallocate(NULL, 0, sizeof(struct obj_string) + length + 1);
  string->length = length;
  string->is_owned = true;
  string->obj.kind = OBJ_KIND_STRING;
  string->hash = hash;

  memcpy(string->as.owned, chars, length);
  string->as.owned[length] = 0;

  string->obj.next = *objects;
  *objects = AS_OBJ(string);

  table_insert(strings, string, TABLE_NIL);

  return string;
}

obj_string_t obj_string_copy(obj_t *objects, table_t *strings,
                             const char *chars, uint32_t length) {
  uint32_t hash = table_hash(chars, length);

  obj_string_t interned = table_find_string(*strings, chars, length, hash);
  if (interned) {
    return interned;
  }

  return _allocate_obj_string(objects, strings, chars, length, hash);
}

obj_string_t obj_string_ref(obj_t *objects, table_t *strings, const char *chars,
                            uint32_t length) {
  uint32_t hash = table_hash(chars, length);
  obj_string_t interned = table_find_string(*strings, chars, length, hash);
  if (interned) {
    return interned;
  }

  obj_string_t string = reallocate(NULL, 0, sizeof(struct obj_string));
  string->is_owned = false;
  string->as.ref = chars;
  string->length = length;
  string->hash = hash;
  string->obj.kind = OBJ_KIND_STRING;

  table_insert(strings, string, TABLE_NIL);

  return string;
}

static void obj_free(obj_t obj) {
  switch (obj->kind) {
  case OBJ_KIND_STRING: {
    obj_string_t string = OBJ_AS_STRING(obj);
    if (string->is_owned) {
      MEM_FREE(string->as.owned, string->length + 1);
    }
    MEM_FREE(obj, sizeof(struct obj));
    break;
  }
  }
}

void objects_free(obj_t *objects) {
  while (*objects) {
    obj_t next = (*objects)->next;
    obj_free(*objects);
    *objects = next;
  }
  objects = NULL;
}

#ifdef DEBUG_OBJ
#include <stdio.h>

void obj_print(const obj_t obj) {
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