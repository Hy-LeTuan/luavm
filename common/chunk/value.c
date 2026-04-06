#include <value.h>
#include <memory.h>

#include <stdio.h>

void initValueArray(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value)
{
    if (array->count + 1 >= array->capacity)
    {
        int newCapacity = GROW_SIZE(array->capacity);
        array->values = REALLOCATE(array->values, array->capacity, newCapacity, Value);
        array->capacity = newCapacity;
    }

    array->values[array->count] = value;
    array->count++;
}

void printValue(Value* value)
{
    switch (value->type)
    {
        case NUMBER:
            fprintf(stdout, "%.2f", AS_NUM(*value));
        case BOOL:
            fprintf(stdout, "%b", AS_BOOL(*value));
    }
}

void printValueNewLine(Value* value)
{
    printValue(value);
    printf("\n");
}

void freeValueArray(ValueArray* array)
{
    FREE(array->values, array->capacity);
    array->capacity = 0;
    array->count = 0;
}
