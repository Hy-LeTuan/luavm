#include <value.h>
#include <memory.h>
#include <object.h>

#include <stdio.h>
#include <string.h>

const Value NIL_CONSTANT = { NIL };

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
    switch (vtype(value))
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
        case OBJ_STRING:
        {
            ObjString* string = (ObjString*)AS_OBJ(value);
            fprintf(stdout, "%s", string->chars);
            break;
        }
        case OBJ_TABLE:
            fprintf(stdout, "table");
            break;
        case OBJ_FUNCTION:
            fprintf(stdout, "function");
            break;
        case OBJ_CLOSURE:
            fprintf(stdout, "closure");
            break;
        case OBJ_NATIVE:
            fprintf(stdout, "native_fn");
            break;
        case OBJ_UPVALUE:
            fprintf(stdout, "upvalue");
            break;
    }
}

void printValueNewLine(Value* value)
{
    printValue(value);
    fprintf(stdout, "\n");
}

bool compareValue(Value* a, Value* b)
{
    if (vtype(a) != vtype(b))
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
bool isFalsey(Value* value)
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
