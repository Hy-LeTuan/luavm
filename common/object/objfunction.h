#ifndef common_object_objfunction_h
#define common_object_objfunction_h

#include <object.h>
#include <chunk.h>

#include <stdint.h>

#define IS_FUNCTION(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction*)(AS_OBJ(value)))

typedef enum
{
    TYPE_FUNCTION,
    TYPE_VARARG,

    /*
       use to comply with Lua 5.1 where both `arg` and `...` are allowed inside a vararg function,
       but not at the same time
    */
    TYPE_VARARG_NO_ARG,
} FunctionType;

typedef struct
{
    Object obj;
    size_t arity;
    uint8_t* ip;
    Chunk chunk;
    int upvalueCount;
    FunctionType type;
} ObjFunction;

ObjFunction* newFunction();

#endif
