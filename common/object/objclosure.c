#include <object.h>
#include <memory.h>

ObjClosure* newClosure(ObjFunction* function, VM* vm)
{
    ObjClosure* closure = ALLOCATE_OBJ(OBJ_CLOSURE, ObjClosure, vm);

    ObjUpvalue** upvalues =
      ALLOCATE(ObjUpvalue*, sizeof(ObjUpvalue*) * function->upvalueCount, NULL);

    for (int i = 0; i < function->upvalueCount; i++)
    {
        upvalues[i] = NULL;
    }

    closure->upvalues = upvalues;
    closure->function = function;

    return closure;
}
