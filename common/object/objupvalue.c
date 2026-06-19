#include <object.h>

ObjUpvalue* newUpvalue(Value* location, VM* vm)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(OBJ_UPVALUE, ObjUpvalue, vm);

    upvalue->location = location;
    upvalue->storage = NIL_CONSTANT;
    upvalue->closed = false;
    upvalue->next = NULL;

    return upvalue;
}
