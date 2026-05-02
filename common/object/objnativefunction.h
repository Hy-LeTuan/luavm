#ifndef common_object_objnativefunction_h
#define common_object_objnativefunction_h

#include <object.h>
#include <value.h>

#define IS_NATIVE(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_NATIVE)
#define AS_NATIVE(value) ((ObjNativeFunction*)(AS_OBJ(value)))

typedef Value (*NativeFn)(Value* args);

typedef struct
{
    Object obj;
    NativeFn function;
} ObjNativeFunction;

ObjNativeFunction* newNativeFunction(NativeFn function);

#endif
