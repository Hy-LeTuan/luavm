#include <objclosure.h>

#include <memory.h>

ObjClosure* newClosure(ObjFunction* function)
{
    ObjClosure* closure = ALLOCATE_OBJ(OBJ_CLOSURE, ObjClosure);

    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, sizeof(ObjUpvalue*) * function->upvalueCount);

    for (int i = 0; i < function->upvalueCount; i++)
    {
        upvalues[i] = NULL;
    }

    closure->upvalues = upvalues;
    closure->function = function;

    return closure;
}
