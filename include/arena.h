#ifndef CAMPSEUDO_ARENA_H
#define CAMPSEUDO_ARENA_H

#include <stdint.h>
#include <stdlib.h>

#define ARENA_ALLOC(arena, type) arena_alloc(arena, sizeof(type))
#define ARENA_ALLOC_ARRAY(arena, type, count)                                  \
  arena_alloc(arena, sizeof(type) * count)

struct arena_chunk {
  struct arena_chunk *next;
  size_t size;
  size_t used;
  uint8_t data[];
};

struct arena {
  struct arena_chunk *curr;
  struct arena_chunk head;
};

struct arena *arena_new(size_t size);
void *arena_alloc(struct arena *arena, size_t size);
void arena_reset(struct arena *arena);
void arena_free(struct arena *arena);

#endif