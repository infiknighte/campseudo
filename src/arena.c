#include "arena.h"

struct arena *arena_new(size_t size) {
  struct arena *arena = malloc(sizeof(struct arena) + size);
  arena->curr = &arena->head;
  arena->head = (struct arena_chunk){.size = size, .used = 0, .next = NULL};
  return arena;
}

void *arena_alloc(struct arena *arena, size_t size) {
  struct arena_chunk *curr = arena->curr;
  size_t new_used = curr->used + size;

  if (new_used >= curr->size) {
    size_t new_size = curr->size * 2;
    curr->next = malloc(sizeof(struct arena_chunk) + new_size);
    if (!curr->next) {
      exit(1);
    }
    arena->curr = curr = curr->next;
    *curr = (struct arena_chunk){.size = new_size, .used = 0, .next = NULL};
  }

  void *ptr = curr->data + curr->used;
  curr->used += size;
  return ptr;
}

void arena_reset(struct arena *arena) {
  struct arena_chunk *node = arena->head.next;
  while (node) {
    node->used = 0;
    node = node->next;
  }
  arena->head.used = 0;
  arena->curr = &arena->head;
}

void arena_free(struct arena *arena) {
  struct arena_chunk *node = arena->head.next;
  while (node) {
    struct arena_chunk *next = node->next;
    free(node);
    node = next;
  }
  free(arena);
}