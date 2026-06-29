#include <value.h>
#include <object.h>

#include <stdio.h>

const Value NIL_CONSTANT = { NIL };
const Value TRUE_CONSTANT = { BOOL, { .boolean = true } };
const Value FALSE_CONSTANT = { BOOL, { .boolean = false } };

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
