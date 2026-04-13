#include <objstring.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>

/*
 * Called when a new string is found. New string will be interned
 * */
static ObjString* newString(char* chars, size_t length, Table* strings)
{
    ObjString* string = ALLOCATE_OBJ(OBJ_STRING, ObjString);
    string->length = length;
    string->chars = chars;

    tableInsert(OBJ_VAL((Object*)string), BOOL_VAL(true), strings);

    return string;
}

ObjString* takeString(char* chars, int length, Table* strings)
{
    Value key = tableFindString(chars, length, strings);
    if (IS_STRING(key))
    {
        return AS_STRING(key);
    }

    return newString(chars, length, strings);
}

ObjString* copyString(const char* const_chars, int length, Table* strings)
{
    Value key = tableFindString(const_chars, length, strings);
    if (IS_STRING(key))
    {
        return AS_STRING(key);
    }

    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, const_chars, length);
    chars[length] = '\0';

    return newString(chars, length, strings);
}

ObjString* concatenateString(ObjString* a, ObjString* b, Table* strings)
{
    size_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length, strings);
    return result;
}
