#ifndef common_memory_memory_h
#define common_memory_memory_h

#include <stddef.h>

#define GROWTH_RATE 4
#define GROW_SIZE(oldSize) (max((oldSize) * 4, 4));

#define REALLOCATE(ptr, oldSize, newSize, type)                                                    \
    ((type*)(reallocate(ptr, sizeof(type) * oldSize, sizeof(type) * newSize)))
#define FREE(ptr, oldSize) (reallocate(ptr, oldSize, 0))

void* reallocate(void* ptr, size_t oldSize, size_t newSize);
size_t max(size_t a, size_t b);

#endif
