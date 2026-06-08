#include <baselib.h>

#include <object.h>

#include <stdio.h>
#include <math.h>

static void printAux(Value arg)
{
    if (IS_NUM(arg))
    {
        float num = AS_NUM(arg);

        if (floor(num) == num)
        {
            fprintf(stdout, "%d", (int)num);
        }
        else
        {
            fprintf(stdout, "%.2f", AS_NUM(arg));
        }
    }
    else if (IS_BOOL(arg))
    {
        fprintf(stdout, "%s", AS_BOOL(arg) ? "true" : "false");
    }
    else if (IS_NIL(arg))
    {
        fprintf(stdout, "nil");
    }
    else if (IS_STRING(arg))
    {
        ObjString* string = v2obj(ObjString, arg);
        fprintf(stdout, "%s", string->chars);
    }
    else if (IS_TABLE(arg))
    {
        fprintf(stdout, "table");
    }
    else if (IS_FUNCTION(arg))
    {
        fprintf(stdout, "function");
    }
    else if (IS_CLOSURE(arg))
    {
        fprintf(stdout, "closure");
    }
    else if (IS_NATIVE(arg))
    {
        fprintf(stdout, "native_fn");
    }
    else if (IS_UPVALUE(arg))
    {
        fprintf(stdout, "upvalue");
    }
}

uint8_t lib_print(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    for (uint8_t i = 0; i < narg; i++)
    {
        printAux(frame->slots[i]);

        if (i != narg - 1)
        {
            printf("\t");
        }
    }

    printf("\n");

    return 0;
}

static uint8_t ipairsAux(uint8_t arg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    Value state = frame->slots[0];
    Value var = frame->slots[1];

    if (!IS_TABLE(state))
    {
        runtimeError(vm, "Error, object is not a table.");
        return 0;
    }
    else if (!IS_NUM(var))
    {
        runtimeError(vm, "Error, index is not a number.");
        return 0;
    }

    ObjTable* table = AS_TABLE(state);
    Value idx = NUM_VAL(AS_NUM(var) + 1);
    Value result = tableGet(idx, &table->content);

    if (IS_NIL(result))
    {
        pushStack(NIL_CONSTANT, vm);
        pushStack(NIL_CONSTANT, vm);
        return 2;
    }
    else
    {
        pushStack(idx, vm);
        pushStack(result, vm);
        return 2;
    }
}

uint8_t lib_ipairs(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    Value table = *frame->slots;

    if (!IS_TABLE(table))
    {
        runtimeError(vm, "Error, object is not a table.");
        return 0;
    }

    ObjNativeFunction* iter = newNativeFunction(ipairsAux);
    linkObject((Object*)iter, vm);

    pushStack(NATIVE_VAL(iter), vm);
    pushStack(table, vm);
    pushStack(NUM_VAL(0), vm);

    return 3;
}

LibExport BASE_LIB[] = { { "print", lib_print }, { "ipairs", lib_ipairs }, { NULL, NULL } };
