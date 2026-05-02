#include <objnativefunction.h>

ObjNativeFunction* newNativeFunction(NativeFn function)
{
    ObjNativeFunction* native = ALLOCATE_OBJ(OBJ_NATIVE, ObjNativeFunction);

    native->function = function;

    return native;
}
