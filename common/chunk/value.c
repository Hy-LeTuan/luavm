#include <value.h>
#include <memory.h>

#include <stdio.h>

void initValueArray(ValueArray* array)
{
    array->ptr = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value)
{
    if (array->count + 1 >= array->capacity)
    {
        int newCapacity = GROW_SIZE(array->capacity);
        array->ptr = REALLOCATE(array->ptr, array->capacity, newCapacity, Value);
        array->capacity = newCapacity;
    }

    array->ptr[array->count] = value;
    array->count++;
}

void printValue(Value* value)
{
    switch (value->type)
    {
        case NUMBER:
            fprintf(stdout, "%f", value->as.number);
        case BOOL:
            fprintf(stdout, "%b", value->as.boolean);
    }
}

void printValueNewLine(Value* value)
{
    printValue(value);
    printf("\n");
}

void freeValueArray(ValueArray* array)
{
    FREE(array->ptr, array->capacity);
    array->capacity = 0;
    array->count = 0;
}
