#include <object.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <hash.h>

/*
 * Called when a new string is found. New string will be interned
 * */
static ObjString* newString(char* chars, size_t length, VM* vm)
{
    ObjString* string = ALLOCATE_OBJ(OBJ_STRING, ObjString, vm);
    string->length = length;
    string->chars = chars;
    string->hash = hashString(chars, length, fnv1a_32);

    tableSet(STRING_VAL(string), TRUE_VAL(), &vm->strings, vm);

    return string;
}

ObjString* takeString(char* chars, int length, VM* vm)
{
    Value key = tableFindString(chars, length, &vm->strings);
    if (IS_STRING(&key))
    {
        return AS_STRING(&key);
    }

    return newString(chars, length, vm);
}

ObjString* copyString(const char* const_chars, int length, VM* vm)
{
    Value key = tableFindString(const_chars, length, &vm->strings);
    if (IS_STRING(&key))
    {
        return AS_STRING(&key);
    }

    char* chars = ALLOCATE(char, length + 1, vm);
    memcpy(chars, const_chars, length);
    chars[length] = '\0';

    return newString(chars, length, vm);
}

ObjString* concatenateString(ObjString* a, ObjString* b, VM* vm)
{
    size_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1, vm);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length, vm);
    return result;
}
