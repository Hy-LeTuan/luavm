#include <value.h>
#include <memory.h>
#include <object.h>
#include <objstring.h>

#include <stdio.h>
#include <string.h>

const Value NIL_CONSTANT = ((Value){ NIL });

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
            fprintf(stdout, "%s", AS_BOOL(value) ? "true" : "false");
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
    if (a.type != b.type)
    {
        return false;
    }
    else
    {
        if (IS_NIL(a))
        {
            return true;
        }
        else if (IS_NUM(a))
        {
            return AS_NUM(a) == AS_NUM(b);
        }
        else if (IS_STRING(a))
        {
            return AS_STRING(a) == AS_STRING(b);
        }
        else if (IS_BOOL(a))
        {
            return AS_BOOL(a) == AS_BOOL(b);
        }
    }

    return false;
}

/*
 * Only `nil` and the boolean `false` are evaulated as false
 * */
bool isFalsey(Value value)
{
    if (IS_NIL(value))
    {
        return true;
    }
    else if (IS_BOOL(value))
    {
        return AS_BOOL(value) ? false : true;
    }

    return false;
}

void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(array->values, array->capacity, Value);
    array->capacity = 0;
    array->count = 0;
}
