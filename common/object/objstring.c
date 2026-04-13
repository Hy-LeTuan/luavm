#include <objstring.h>

#include <string.h>
#include <memory.h>

#include <stdio.h>

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
    printf("reach copyString\n");

    Value key = tableFindString(const_chars, length, strings);

    printf("already got key.\n");

    if (IS_STRING(key))
    {
        return AS_STRING(key);
    }

    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, const_chars, length);
    chars[length] = '\0';

    return newString(chars, length, strings);
}
