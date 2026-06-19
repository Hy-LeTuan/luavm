#include <gc.h>

#include <memory.h>
#include <object.h>
#include <chunk.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif

void freeObject(Object* obj, VM* vm)
{
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, vtype(obj));
#endif

    if (obj == NULL)
    {
        return;
    }

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
            FREE(function, ObjFunction, vm);

            for (int i = 0; i < function->esize; i++)
            {
                freeObject(baseobj(function->enclosed[i]), vm);
                function->enclosed[i] = NULL;
            }
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE_ARRAY(closure->upvalues, closure->function->upvalueCount, ObjUpvalue, vm);
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

            freeObject(baseobj(table->mt), vm);
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

// should everything be bound by the VM?
// if it should, then everything has to happened when the VM is active, which admittedly looks a bit
// riddiculous
void collectGarbage(VM* vm)
{
    // skip gc for this allocationn
    if (vm == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("--- gc begin\n");
#endif

    markRoots();

#ifdef DEBUG_LOG_GC
    printf("--- gc end\n");
#endif
}
