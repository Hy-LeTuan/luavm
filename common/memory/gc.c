#include <gc.h>

#include <memory.h>
#include <objstring.h>

#include <stdio.h>

static void freeObject(Object* obj)
{
    switch (obj->type)
    {
        case OBJ_STRING:
        {
            ObjString* string = (ObjString*)obj;
            FREE_ARRAY(string->chars, string->length + 1, char);
            FREE(string, ObjString);
        }
        case OBJ_TABLE:
            break;
        case OBJ_FUNCTION:
            break;
    }
}

void freeObjects(Object* objects)
{
    Object* p = objects;

    while (p != NULL)
    {
        Object* next = p->next;
        freeObject(p);
        p = next;
    }
}
