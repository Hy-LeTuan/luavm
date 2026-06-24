#include <memory.h>

#include <gc.h>

#include <stdio.h>
#include <stdlib.h>

void* reallocate(void* ptr, size_t oldSize, size_t newSize, VM* vm)
{
    if (newSize > oldSize)
    {
        vm->bytesAllocated += newSize - oldSize;
        if (vm->bytesAllocated > vm->GCthreshold)
        {
            collectGarbage(vm);
        }
#ifdef DEBUG_STRESS_GC
        collectGarbage(vm);
#endif
    }

    if (newSize == oldSize)
    {
        return ptr;
    }
    else if (newSize == 0)
    {
        free(ptr);
        return NULL;
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
