#ifndef common_object_object_h
#define common_object_object_h

#define IS_OBJ(value) ((value).type == OBJECT)
#define AS_OBJ(value) ((value).as.object)
#define OBJ_VAL(obj) ((Value){ OBJECT, { .object = obj } })

#define IS_STRING(value) (IS_OBJ(value) && AS_OBJ(value)->type == OBJ_STRING)
#define AS_STRING(value) ((ObjString*)(AS_OBJ(value)))

#define ALLOCATE_OBJ(objType, objStruct) ((objStruct*)allocateObj(objType, sizeof(objStruct)))

#include <stddef.h>

typedef enum
{
    OBJ_STRING,
    OBJ_TABLE,
    OBJ_FUNCTION
} ObjType;

typedef struct Object
{
    ObjType type;
    struct Object* next;
} Object;

typedef struct
{
    Object obj;
    size_t length;
    char* chars;
} ObjString;

Object* allocateObj(ObjType type, size_t size);

// string functions
ObjString* newString(char* chars, size_t length);
ObjString* copyString(const char* const_chars, int length);
ObjString* takeString(char* chars, int length);

void printObject(Object* obj);

#endif
