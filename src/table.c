#include "table.h"
#include "memory.h"
#include "obj.h"
#include "value.h"
#include <stdint.h>
#include <string.h>

#define CAPACITY_INIT 8
#define CAPACITY_MULT 2
#define TABLE_MAX_LOAD 0.75F

struct table *table_new(void) {
  struct table *table =
      MEM_ALLOC(sizeof(struct table) + CAPACITY_INIT * sizeof(struct entry));
  table->count = 0;
  table->capacity = CAPACITY_INIT;

  struct entry *entries = table->entries;
  for (uint32_t i = 0; i < CAPACITY_INIT; ++i) {
    entries[i].key = NULL;
    entries[i].value = TABLE_NIL;
  }

  return table;
}

uint32_t table_hash(const uint8_t *key, uint32_t length) {
  uint32_t hash = 2166136261U;
  for (uint32_t i = 0; i < length; i++) {
    hash ^= key[i];
    hash *= 16777619;
  }
  return hash;
}

static struct entry *_find_entry(struct entry *entries, uint32_t capacity,
                                 const struct obj_string *key) {
  uint32_t index = key->hash % capacity;
  struct entry *tombstone = NULL;

  for (;;) {
    struct entry *entry = entries + index;
    if (!entry->key) {
      if (VALUE_AS_BOOL(entry->value)) {
        if (!tombstone) {
          tombstone = entry;
        }
      } else {
        return (tombstone) ? tombstone : entry;
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void _adjust_capacity(struct table **table, uint32_t capacity) {
  struct table *self = *table;
  struct table *new_table =
      MEM_ALLOC(sizeof(struct table) + capacity * sizeof(struct entry));
  if (!new_table) {
  }
  new_table->capacity = capacity;
  new_table->count = 0;

  struct entry *new_entries = new_table->entries;
  for (uint32_t i = 0; i < capacity; ++i) {
    new_entries[i].key = NULL;
    new_entries[i].value = TABLE_NIL;
  }

  struct entry *old_entries = self->entries;
  uint32_t old_capacity = self->capacity;
  for (uint32_t i = 0; i < old_capacity; ++i) {
    struct entry *entry = old_entries + i;
    if (!entry) {
      continue;
    }

    struct entry *dest = _find_entry(new_entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    ++new_table->count;
  }

  MEM_FREE(*table, sizeof(struct table) + old_capacity * sizeof(struct entry));
  *table = new_table;
}

bool table_insert(struct table **table, struct obj_string *key,
                  struct value value) {
  struct table *self = *table;

  if (self->count >= self->capacity * TABLE_MAX_LOAD) {
    uint32_t capacity = self->capacity * CAPACITY_MULT;
    _adjust_capacity(table, capacity);
    self = *table;
  }

  struct entry *entry = _find_entry(self->entries, self->capacity, key);

  bool is_new = !entry->key && !VALUE_AS_BOOL(entry->value);

  entry->key = key;
  entry->value = value;

  if (is_new) {
    ++self->count;
    return true;
  }

  return false;
}

void table_add_all(const struct table *from, struct table **to) {
  for (uint32_t i = 0; i < from->capacity; ++i) {
    const struct entry *entry = from->entries + i;
    if (entry->key) {
      table_insert(to, entry->key, entry->value);
    }
  }
}

bool table_member(const struct table *table, const struct obj_string *key,
                  struct value *value) {
  if (!table->count) {
    return false;
  }

  const struct entry *entry =
      _find_entry((struct entry *)table->entries, table->capacity, key);
  if (!entry->key) {
    return false;
  }

  *value = entry->value;
  return true;
}

bool table_delete(struct table *table, const struct obj_string *key) {
  if (!table->count) {
    return false;
  }

  struct entry *entry = _find_entry(table->entries, table->capacity, key);
  if (!entry->key) {
    return false;
  }

  entry->key = NULL;
  entry->value = VALUE_FROM_BOOL(true);

  return true;
}

struct obj_string *table_find_string(const struct table *table,
                                     const uint8_t *chars, uint32_t length,
                                     uint32_t hash) {
  if (!table->count) {
    return NULL;
  }

  uint32_t index = hash % table->capacity;
  for (;;) {
    const struct entry *entry = table->entries + index;

    if (!entry->key) {
      if (!VALUE_AS_BOOL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->length == length && entry->key->hash == hash &&
               !memcmp(OBJ_AS_CSTRING(entry->key), chars, length)) {
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}
