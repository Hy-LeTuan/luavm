#include <stringlib.h>

uint8_t lib_len(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value v = frame->slots[0];

    if (!IS_STRING(v))
    {
        return 0;
    }

    ObjString* str = AS_STRING(v);
    pushStack(NUM_VAL(str->length), vm);

    return 1;
}

LibExport STRING_LIB[] = { { "len", lib_len }, { NULL, NULL } };
