#include <object.h>
#include <memory.h>

ObjClosure* newClosure(ObjFunction* function, VM* vm)
{
    ObjClosure* closure = ALLOCATE_OBJ(OBJ_CLOSURE, ObjClosure, vm);
    closure->function = function;
    closure->upvalues = NULL;

    unsafe_push(vm, CLOSURE_VAL(closure));

    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, sizeof(ObjUpvalue*) * function->upvalueCount, vm);

    for (int i = 0; i < function->upvalueCount; i++)
    {
        upvalues[i] = NULL;
    }

    closure->upvalues = upvalues;

    unsafe_pop(vm);

    return closure;
}
