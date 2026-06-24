#include <object.h>
#include <memory.h>
#include <gc.h>

ObjClosure* newClosure(ObjFunction* function, VM* vm)
{
    ObjClosure* closure = ALLOCATE_OBJ(OBJ_CLOSURE, ObjClosure, vm);
    closure->function = function;
    closure->upvalues = NULL;

    lock_object(baseobj(closure));

    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, sizeof(ObjUpvalue*) * function->upvalueCount, vm);

    for (int i = 0; i < function->upvalueCount; i++)
    {
        upvalues[i] = NULL;
    }

    closure->upvalues = upvalues;

    release_object(baseobj(closure));

    return closure;
}
