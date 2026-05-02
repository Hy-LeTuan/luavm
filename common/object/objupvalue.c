#include <objupvalue.h>

ObjUpvalue* newUpvalue(Value* location)
{
    ObjUpvalue* upvalue = ALLOCATE_OBJ(OBJ_UPVALUE, ObjUpvalue);

    upvalue->location = location;
    upvalue->storage = NIL_CONSTANT;
    upvalue->closed = false;
    upvalue->next = NULL;

    return upvalue;
}
