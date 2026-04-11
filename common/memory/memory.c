#include <memory.h>

#include <stdio.h>
#include <stdlib.h>

void* reallocate(void* ptr, size_t oldSize, size_t newSize)
{
    if (newSize == oldSize)
    {
        return ptr;
    }

    void* newPtr = realloc(ptr, newSize);

    if (newSize != 0 && newPtr == NULL)
    {
        fprintf(stderr, "Error in allocating memory for luavm.\n");
        exit(EXIT_FAILURE);
    }

    return newPtr;
}

size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}
