#ifndef vm_vm_h
#define vm_vm_h

#include <chunk.h>
#include <value.h>
#include <object.h>
#include <table.h>
#include <objclosure.h>

#define STACK_MAX 256
#define CACHE_MAX 128

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_ERROR
} InterpretResult;

typedef struct
{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
    // expected number of return value
    uint8_t expected;
} CallFrame;

typedef struct
{
    Value stack[STACK_MAX];
    Value cache[CACHE_MAX];
    uint8_t cacheSize;
    CallFrame frames[STACK_MAX];
    int frameCount;
    Value* stackTop;
    Object* objectStack;
    Table strings;
    Table globals;
    ObjUpvalue* openUpvalues;
} VM;

void initVM(VM* vm);
InterpretResult interpret(const char* source);
InterpretResult run(VM* vm);
void linkObject(Object* obj, VM* vm);
void freeVM(VM* vm);

#endif
