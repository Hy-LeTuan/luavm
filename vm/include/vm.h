#ifndef vm_vm_h
#define vm_vm_h

#include <chunk.h>
#include <value.h>

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
} VM;

void initVM(VM* vm);
InterpretResult interpret(const char* source);
void freeVM(VM* vm);

#endif
