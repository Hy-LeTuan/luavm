#include <stringlib.h>

#include <memory.h>

#include <ctype.h>
#include <string.h>

uint8_t lib_len(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* v = &frame->slots[0];

    if (!IS_STRING(v))
    {
        return 0;
    }

    ObjString* str = AS_STRING(v);
    pushStack(NUM_VAL(str->length), vm);

    return 1;
}

uint8_t lib_lower(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* v = &frame->slots[0];

    if (!IS_STRING(v))
    {
        return 0;
    }

    ObjString* str = AS_STRING(v);
    char* chars = ALLOCATE(char, str->length + 1, vm);

    for (uint8_t i = 0; i < str->length; i++)
    {
        if (isupper(str->chars[i]))
        {
            chars[i] = 'a' + (str->chars[i] - 'A');
        }
        else
        {
            chars[i] = str->chars[i];
        }
    }
    chars[str->length] = '\0';

    ObjString* new = takeString(chars, str->length, vm);
    pushStack(STRING_VAL(new), vm);

    return 1;
}

uint8_t lib_upper(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* v = &frame->slots[0];

    if (!IS_STRING(v))
    {
        return 0;
    }

    ObjString* str = AS_STRING(v);
    char* chars = ALLOCATE(char, str->length + 1, vm);

    for (uint8_t i = 0; i < str->length; i++)
    {
        if (islower(str->chars[i]))
        {
            chars[i] = 'A' + (str->chars[i] - 'a');
        }
        else
        {
            chars[i] = str->chars[i];
        }
    }
    chars[str->length] = '\0';

    ObjString* new = takeString(chars, str->length, vm);

    pushStack(STRING_VAL(new), vm);

    return 1;
}

uint8_t lib_rep(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* arg1 = &frame->slots[0];
    Value* arg2 = &frame->slots[1];

    if (!IS_STRING(arg1) || !IS_NUM(arg2))
    {
        return 0;
    }

    ObjString* str = AS_STRING(arg1);
    int n = AS_NUM(arg2);

    char* chars = ALLOCATE(char, (str->length) * n + 1, vm);

    for (uint8_t i = 0; i < n; i++)
    {
        memcpy(chars + i * str->length, str->chars, str->length);
    }

    chars[str->length * n] = '\0';

    ObjString* new = takeString(chars, str->length * n, vm);

    pushStack(STRING_VAL(new), vm);

    return 1;
}

uint8_t lib_reverse(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* v = &frame->slots[0];

    if (!IS_STRING(v))
    {
        return 0;
    }

    ObjString* str = AS_STRING(v);
    char* chars = ALLOCATE(char, str->length + 1, vm);

    for (uint8_t i = 0; i < str->length; i++)
    {
        chars[i] = str->chars[str->length - i - 1];
    }
    chars[str->length] = '\0';

    ObjString* new = takeString(chars, str->length, vm);

    pushStack(STRING_VAL(new), vm);

    return 1;
}

LibExport STRING_LIB[] = { { "len", lib_len }, { "lower", lib_lower }, { "upper", lib_upper },
    { "reverse", lib_reverse }, { "rep", lib_rep }, { NULL, NULL } };
