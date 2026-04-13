#include <object.h>

#include <objstring.h>
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
