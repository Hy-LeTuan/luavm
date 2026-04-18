#include <locals.h>

#include <memory.h>

void initLocalArray(LocalArray* array)
{
    array->locals = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeLocalArray(LocalArray* array, const char* start, int length, int scope)
{
    if (array->count + 1 >= array->capacity)
    {
        int newCapacity = GROW_SIZE(array->capacity);
        array->locals = REALLOCATE(array->locals, array->capacity, newCapacity, Local);
        array->capacity = newCapacity;
    }

    Local* local = &array->locals[array->count];
    local->start = start;
    local->length = length;
    local->scope = scope;

    array->count++;
}

void freeLocalArray(LocalArray* array)
{
    FREE_ARRAY(array->locals, array->capacity, Local);
    array->capacity = 0;
    array->count = 0;
}
