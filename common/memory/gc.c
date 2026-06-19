#include <gc.h>

#include <memory.h>
#include <object.h>
#include <chunk.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include <stdlib.h>
#endif

void freeObject(Object* obj, VM* vm)
{
    if (obj == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, vtype(obj));
#endif

    switch (otype(obj))
    {
        case OBJ_STRING:
        {
            ObjString* string = castobjto(ObjString, obj);
            FREE_ARRAY(string->chars, string->length + 1, char, vm);
            FREE(string, ObjString, vm);
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)obj;
            freeChunk(&function->chunk, vm);
            FREE_ARRAY(function->enclosed, function->esize, ObjFunction*, vm);
            FREE(function, ObjFunction, vm);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE_ARRAY(closure->upvalues, closure->function->upvalueCount, ObjUpvalue*, vm);
            FREE(closure, ObjClosure, vm);
            break;
        }
        case OBJ_UPVALUE:
        {
            FREE(obj, ObjUpvalue, vm);
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(obj, ObjNativeFunction, vm);
            break;
        }
        case OBJ_TABLE:
        {
            ObjTable* table = (ObjTable*)obj;
            freeTable(&table->content);
            freeValueArray(&table->array);
            FREE(table, ObjTable, vm);
            break;
        }
        default:
            return;
    }
}

void freeObjects(Object* objects, VM* vm)
{
    Object* p = objects;

    while (p != NULL)
    {
        Object* next = p->next;
        freeObject(p, vm);
        p = next;
    }
}

// need a way to access the VM
static void markRoots()
{
}

void collectGarbage(VM* vm)
{
#ifdef DEBUG_LOG_GC
    /* vm should never be null */
    if (vm == NULL)
    {
        fprintf(stderr, "Error, vm is null when passed to gc.\n");
        exit(EXIT_FAILURE);
    }

    printf("--- gc begin\n");
#endif

    markRoots();

#ifdef DEBUG_LOG_GC
    printf("--- gc end\n");
#endif
}
