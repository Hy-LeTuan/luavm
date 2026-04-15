#ifndef vm_vm_h
#define vm_vm_h

#include <chunk.h>
#include <value.h>
#include <object.h>
#include <table.h>

#define STACK_MAX 256

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_ERROR
} InterpretResult;

typedef struct
{
    Value stack[STACK_MAX];
    Chunk chunk;
    Value* stackTop;
    Object* objectStack;
    Table strings;
    Table globals;
} VM;

void initVM(VM* vm);
InterpretResult interpret(const char* source);
InterpretResult run(VM* vm);
void linkObject(Object* obj, VM* vm);
void freeVM(VM* vm);

#endif
