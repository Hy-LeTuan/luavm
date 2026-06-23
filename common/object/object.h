#ifndef common_object_object_h
#define common_object_object_h

#define ALLOCATE_OBJ(valueType, objStruct, vm)                                                     \
    ((objStruct*)allocateObj(valueType, sizeof(objStruct), vm))

#define TABLE(t) (&t->content)

#define baseobj(obj) ((Object*)obj)
#define castobjto(type, obj) ((type*)(obj))

#include <table.h>

#include <stddef.h>
#include <stdint.h>

typedef uint8_t (*NativeFn)(uint8_t nargs, VM* args);

typedef struct Object
{
    ValueType type;
    bool marked;
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
    uint32_t hash;
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
    size_t capacity;
    size_t count;
    uint8_t* code;
    size_t* lines;

    // size and count of constants array
    size_t csize;
    size_t ccount;
    Value* constants;
} Chunk;

typedef struct ObjFunction
{
    Object obj;
    size_t arity;
    uint8_t* ip;
    Chunk chunk;
    int upvalueCount;
    FunctionType type;

    /*
       keeping track of all functions defined inside it. this should be marked during the mark and
       sweep of the GC, and when it finally goes out, everything here should be freed.
    */
    struct ObjFunction** enclosed;
    int esize;
} ObjFunction;

/*
   Lua tables are split into:

   1. An array part for dense positive integer keys.
   2. A hash part for all other keys and sparse integer keys.

   Integer keys that fit efficiently into the array region are stored
   in the array part; everything else is stored in the hash part.

   The length operation also has to follow the boundary search in Lua
*/
typedef struct
{
    // stores the overall capacity
    size_t capacity;
    // stores the index of the next empty position
    size_t count;
    Value* values;
} ValueArray;

typedef struct ObjTable
{
    Object obj;
    struct ObjTable* mt;
    ValueArray array;
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

Object* allocateObj(ValueType type, size_t size, VM* vm);

/* string functions */
ObjString* copyString(const char* const_chars, int length, VM* vm);
ObjString* takeString(char* chars, int length, VM* vm);
ObjString* concatenateString(ObjString* a, ObjString* b, VM* vm);

/* table functions */
ObjTable* newObjTable(VM* vm);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value, VM* vm);
void freeValueArray(ValueArray* array, VM* vm);

/* functions and closures */
ObjNativeFunction* newNativeFunction(NativeFn function, VM* vm);
ObjFunction* newFunction(ObjFunction* parent, VM* vm);
ObjClosure* newClosure(ObjFunction* function, VM* vm);

/* upvalue */
ObjUpvalue* newUpvalue(Value* location, VM* vm);

#endif
