#include "obj.h"
#include "common.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

#define ALLOCATE_OBJ(type, kind) (type *)_allocate_obj(kind, sizeof(type))

obj_t _allocate_obj(obj_t *objects, enum obj_kind kind, size_t size) {
  obj_t obj = reallocate(NULL, 0, size);
  obj->kind = kind;
  obj->next = *objects;
  *objects = obj;
  return obj;
}

obj_string_t obj_string_new(uint32_t length) {
  obj_string_t string =
      reallocate(NULL, 0, sizeof(struct obj_string) + length + 1);
  string->length = length;
  string->is_owned = true;
  string->as.owned[length] = 0;
  string->obj.kind = OBJ_KIND_STRING;
  return string;
}

obj_string_t obj_string_copy(const char *chars, uint32_t length) {
  obj_string_t string = obj_string_new(length);
  memcpy(string->as.owned, chars, length);
  return string;
}

obj_string_t obj_string_ref(const char *chars, uint32_t length) {
  obj_string_t string = reallocate(NULL, 0, sizeof(struct obj_string));
  string->is_owned = false;
  string->as.ref = chars;
  string->length = length;
  string->obj.kind = OBJ_KIND_STRING;
  return string;
}

static void obj_free(obj_t object) {
  switch (object->kind) {
  case OBJ_KIND_STRING: {
    obj_string_t string = OBJ_AS_STRING(object);
    if (string->is_owned) {
      MEM_FREE(char, string->as.owned, string->length + 1);
    }
    MEM_FREE(struct obj_string, object, 1);
    break;
  }
  }
}

void objects_free(obj_t obj) {
  while (obj) {
    obj_t next = obj->next;
    obj_free(obj);
    obj = next;
  }
  obj->next = NULL;
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