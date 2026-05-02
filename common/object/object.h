#ifndef common_object_object_h
#define common_object_object_h

#define IS_OBJ(value) ((value).type == OBJECT)
#define AS_OBJ(value) ((value).as.object)
#define OBJ_VAL(obj) ((Value){ OBJECT, { .object = obj } })

#define ALLOCATE_OBJ(objType, objStruct) ((objStruct*)allocateObj(objType, sizeof(objStruct)))

#include <stddef.h>

typedef enum
{
    OBJ_STRING,
    OBJ_TABLE,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE
} ObjType;

typedef struct Object
{
    ObjType type;
    struct Object* next;
} Object;

Object* allocateObj(ObjType type, size_t size);
void printObject(Object* obj);

#endif
