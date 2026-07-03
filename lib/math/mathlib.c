#include <mathlib.h>

#include <math.h>
#include <stdlib.h>
#include <utils.h>

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

uint8_t lib_random(uint8_t nargs, VM* vm)
{
    CallFrame* frame = currframe(vm);

    int raw = rand();

    if (nargs == 0)
    {
        pushStack(NUM_VAL(raw), vm);
    }
    else
    {
        Value* arg = &frame->slots[0];

        if (IS_NUM(arg))
        {
            if (nargs == 1 && AS_NUM(arg) == 0)
            {
                pushStack(NUM_VAL(raw), vm);
            }

            LuaNum lower = AS_NUM(arg);

            if (nargs == 2)
            {
                Value* arg2 = &frame->slots[1];

                if (IS_NUM(arg2))
                {
                    LuaNum upper = AS_NUM(arg2);
                    LuaNum diff = upper - lower;

                    if (diff == 0)
                    {
                        pushStack(NUM_VAL(lower), vm);
                    }
                    else
                    {
                        LuaNum mod = MOD(raw, diff);
                        pushStack(NUM_VAL(lower + mod), vm);
                    }
                }
            }
            else
            {
                pushStack(NUM_VAL(raw % (int)lower), vm);
            }
        }
    }

    return 1;
}

LibExport MATH_LIB[] = { { "floor", lib_floor }, { "random", lib_random }, { NULL, NULL } };
