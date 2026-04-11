#include <value.h>
#include <memory.h>
#include <object.h>

#include <stdio.h>
#include <string.h>

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

void printValue(Value value)
{
    switch (value.type)
    {
        case NUMBER:
            fprintf(stdout, "%.2f", AS_NUM(value));
            break;
        case BOOL:
            fprintf(stdout, "%b", AS_BOOL(value));
            break;
        case NIL:
            fprintf(stdout, "nil");
            break;
        case OBJECT:
            printObject(AS_OBJ(value));
            break;
    }
}

void printValueNewLine(Value value)
{
    printValue(value);
    fprintf(stdout, "\n");
}

bool compareValue(Value a, Value b)
{
    if (IS_BOOL(a) && IS_BOOL(b))
    {
        return AS_BOOL(a) == AS_BOOL(b);
    }
    else if (IS_NUM(a) && IS_NUM(b))
    {
        return AS_NUM(a) == AS_NUM(b);
    }
    else if (IS_NIL(a) && IS_NIL(b))
    {
        return true;
    }
    else if (IS_STRING(a) == IS_STRING(b))
    {
        ObjString* a_str = AS_STRING(a);
        ObjString* b_str = AS_STRING(b);

        return a_str->length == b_str->length &&
          memcmp(a_str->chars, b_str->chars, a_str->length) == 0;
    }

    return false;
}

void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(array->values, array->capacity, Value);
    array->capacity = 0;
    array->count = 0;
}
