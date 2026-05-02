#include <gc.h>

#include <memory.h>
#include <objstring.h>
#include <objfunction.h>
#include <objclosure.h>
#include <objnativefunction.h>

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
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)obj;
            freeChunk(&function->chunk);
            FREE(function, ObjFunction);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE_ARRAY(closure->upvalues, closure->function->upvalueCount, ObjUpvalue);
            break;
        }
        case OBJ_UPVALUE:
        {
            FREE(obj, ObjUpvalue);
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(obj, ObjNativeFunction);
            break;
        }
        case OBJ_TABLE:
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
