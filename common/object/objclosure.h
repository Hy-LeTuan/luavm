#ifndef common_object_objclosure_h
#define common_object_objclosure_h

#include <object.h>
#include <objfunction.h>
#include <objupvalue.h>

#define IS_CLOSURE(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_CLOSURE)
#define AS_CLOSURE(value) ((ObjClosure*)(AS_OBJ(value)))

typedef struct
{
    Object obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
} ObjClosure;

ObjClosure* newClosure(ObjFunction* function);

#endif
