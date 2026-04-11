#include <object.h>
#include <memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Object* allocateObj(ObjType type, size_t size)
{
    Object* obj = ALLOCATE(Object, size);
    obj->type = type;
    obj->next = NULL;

    return obj;
}

ObjString* newString(char* chars, size_t length)
{
    ObjString* string = ALLOCATE_OBJ(OBJ_STRING, ObjString);
    string->length = length;
    string->chars = chars;

    return string;
}

ObjString* takeString(char* chars, int length)
{
    if (length <= 0 || chars == NULL)
    {
        return newString(NULL, 0);
    }

    // something something intern string here
    return newString(chars, length);
}

ObjString* copyString(const char* const_chars, int length)
{
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, const_chars, length);
    chars[length] = '\0';

    return takeString(chars, length);
}

void printObject(Object* obj)
{
    switch (obj->type)
    {
        case OBJ_STRING:
        {
            ObjString* string = (ObjString*)obj;
            fprintf(stdout, "'%s'", string->chars);
            break;
        }
        case OBJ_TABLE:
            fprintf(stdout, "table");
            break;
        case OBJ_FUNCTION:
            fprintf(stdout, "function");
            break;
    }
}
