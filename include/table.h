#ifndef CAMPSEUDO_TABLE_H
#define CAMPSEUDO_TABLE_H

#include "memory.h"
#include "value.h"
#include <stdint.h>

#define TABLE_NIL VALUE_FROM_BOOL(false)

struct entry {
  struct obj_string *key;
  struct value value;
};

struct table {
  uint32_t count, capacity;
  struct entry entries[];
};

struct table *table_new(void);
uint32_t table_hash(const uint8_t *key, uint32_t length);
bool table_insert(struct table **table, struct obj_string *key,
                  struct value value);
bool table_member(const struct table *table, const struct obj_string *key,
                  struct value *value);
bool table_delete(struct table *table, const struct obj_string *key);
void table_add_all(const struct table *from, struct table **to);
struct obj_string *table_find_string(const struct table *table,
                                     const uint8_t *chars, uint32_t length,
                                     uint32_t hash);

static inline void table_free(struct table *table) {
  MEM_FREE(table,
           sizeof(struct table) + table->capacity * sizeof(struct entry));
}

#endif