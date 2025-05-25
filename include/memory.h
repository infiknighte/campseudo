#ifndef CAMPSEUDO_MEMORY_H
#define CAMPSEUDO_MEMORY_H

#include <stdlib.h>

#define MEM_ARRAY_ALLOC(type, new_size)                                        \
  reallocate(NULL, 0, sizeof(type) * new_size)
#define MEM_ARRAY_REALLOC(type, pointer, old_size, new_size)                   \
  reallocate(pointer, sizeof(type) * old_size, sizeof(type) * new_size)
#define MEM_ARRAY_FREE(type, pointer, old_size)                                \
  reallocate(pointer, sizeof(type) * old_size, 0)

#define MEM_ALLOC(size) reallocate(NULL, 0, size)
#define MEM_FREE(pointer, size) reallocate(pointer, size, 0)

void *reallocate(void *pointer, size_t old_size, size_t new_size);

#endif