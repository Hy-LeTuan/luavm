#ifndef common_object_objfunction_h
#define common_object_objfunction_h

#include <object.h>
#include <chunk.h>

#include <stdint.h>

#define IS_FUNCTION(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction*)(AS_OBJ(value)))

typedef struct
{
    Object obj;
    size_t arity;
    uint8_t* ip;
    Chunk chunk;
    int upvalueCount;
} ObjFunction;

ObjFunction* newFunction();

#endif
