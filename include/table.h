#ifndef CAMPSEUDO_TABLE_H
#define CAMPSEUDO_TABLE_H

#include "value.h"
#include <stdint.h>

#define TABLE_NIL VALUE_FROM_BOOL(false)

typedef struct obj_string *obj_string_t;

struct entry {
  obj_string_t key;
  struct value value;
};

typedef struct table {
  uint32_t count, capacity;
  struct entry entries[];
} *table_t;

void table_init(table_t *table);
void table_free(table_t *table);
uint32_t table_hash(const char *key, uint32_t length);
bool table_insert(table_t *table, struct obj_string *key, struct value value);
bool table_member(const struct table *table, const struct obj_string *key,
                  struct value *value);
bool table_delete(table_t table, const struct obj_string *key);
void table_add_all(const struct table *from, table_t *to);
obj_string_t table_find_string(table_t table, const char *chars,
                               uint32_t length, uint32_t hash);

#endif