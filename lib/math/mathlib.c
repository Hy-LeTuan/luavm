#include <mathlib.h>

#include <math.h>

uint8_t lib_floor(uint8_t nargs, VM* vm)
{
    CallFrame* frame = currframe(vm);

    if (!IS_NUM(frame->slots[0]))
    {
        return 0;
    }

    double num = AS_NUM(frame->slots[0]);
    int res = floor(num);

    pushStack(NUM_VAL(res), vm);

    return 1;
}

LibExport MATH_LIB[] = { { "floor", lib_floor }, { NULL, NULL } };
