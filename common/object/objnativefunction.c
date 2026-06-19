#include <object.h>

ObjNativeFunction* newNativeFunction(NativeFn function, VM* vm)
{
    ObjNativeFunction* native = ALLOCATE_OBJ(OBJ_NATIVE, ObjNativeFunction, vm);

    native->function = function;

    return native;
}
