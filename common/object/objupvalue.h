#ifndef common_object_objupvalue_h
#define common_object_objupvalue_h

#include <object.h>
#include <value.h>

#define IS_UPVALUE(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_UPVALUE)
#define AS_UPVALUE(value) ((ObjUpvalue*)(AS_OBJ(value)))

typedef struct ObjUpvalue
{
    Object obj;

    // intrusive linking
    struct ObjUpvalue* next;

    // the indirection to dynamically point to whether the upvalue is on the stack or on the heap
    Value* location;

    // the heap to store the upvalue
    Value storage;

    // if the upvalue has been moved out of stack
    bool closed;

} ObjUpvalue;

ObjUpvalue* newUpvalue(Value* location);

#endif
