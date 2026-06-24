#ifndef common_memory_memory_h
#define common_memory_memory_h

#include <stddef.h>
#include <vmstate.h>

#define GROWTH_RATE 4
#define GROW_SIZE(oldSize) (max((oldSize) * 4, 4));
#define GC_HEAP_GROW_FACTOR 2

#define REALLOCATE(ptr, oldSize, newSize, type, vm)                                                \
    ((type*)(reallocate(ptr, sizeof(type) * (oldSize), sizeof(type) * (newSize), vm)))
#define ALLOCATE(type, size, vm) ((type*)reallocate(NULL, 0, size, vm))
#define FREE(ptr, type, vm) (reallocate(ptr, sizeof(type), 0, vm))
#define FREE_ARRAY(ptr, length, type, vm) (reallocate(ptr, sizeof(type) * (length), 0, vm))

void* reallocate(void* ptr, size_t oldSize, size_t newSize, VM* vm);
size_t max(size_t a, size_t b);

#endif
