#ifndef common_object_object_h
#define common_object_object_h

#define ALLOCATE_OBJ(valueType, objStruct) ((objStruct*)allocateObj(valueType, sizeof(objStruct)))

#define baseobj(obj) ((Object*)obj)
#define castobjto(type, obj) ((type*)(obj))

#include <table.h>
#include <chunk.h>

#include <stddef.h>
#include <stdint.h>

typedef struct VM VM;
typedef uint8_t (*NativeFn)(uint8_t nargs, VM* args);

typedef struct Object
{
    ValueType type;
    struct Object* next;
} Object;

typedef struct
{
    Object obj;
    NativeFn function;
} ObjNativeFunction;

typedef struct
{
    Object obj;
    size_t length;
    char* chars;
} ObjString;

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

typedef struct
{
    Object obj;
    Table content;
} ObjTable;

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

typedef struct
{
    Object obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
} ObjClosure;

Object* allocateObj(ValueType type, size_t size);

/* string functions */
ObjString* copyString(const char* const_chars, int length, Table* strings);
ObjString* takeString(char* chars, int length, Table* strings);
ObjString* concatenateString(ObjString* a, ObjString* b, Table* strings);

/* table functions */
ObjTable* newTable();

/* functions and closures */
ObjNativeFunction* newNativeFunction(NativeFn function);
ObjFunction* newFunction();
ObjClosure* newClosure(ObjFunction* function);

/* upvalue */
ObjUpvalue* newUpvalue(Value* location);

#endif
