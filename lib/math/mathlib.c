#include <mathlib.h>

#include <math.h>

uint8_t lib_floor(uint8_t nargs, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* arg = &frame->slots[0];

    if (!IS_NUM(arg))
    {
        return 0;
    }

    LuaNum num = AS_NUM(arg);
    int res = floor(num);

    pushStack(NUM_VAL(res), vm);

    return 1;
}

LibExport MATH_LIB[] = { { "floor", lib_floor }, { NULL, NULL } };
