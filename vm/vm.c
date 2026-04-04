#include <vm.h>
#include <compiler.h>

#include <stdio.h>

void initVM(VM* vm)
{
    vm->stackTop = &vm->stack[0];
    initChunk(&vm->chunk);
}

static void push(Value value, VM* vm)
{
    if (vm->stackTop - vm->stack == STACK_MAX)
    {
        fprintf(stderr, "Error, stack limit exceeded.\n");
        exit(EXIT_FAILURE);
    }

    *vm->stackTop = value;
    vm->stackTop++;
}

static void pop(VM* vm)
{
    vm->stackTop--;
}

static InterpretResult run(VM* vm)
{
    uint8_t* ip = vm->chunk.code;

#define READ_BYTE() (*ip++)
    while (true)
    {
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_ADD:
                // assumes that the value is already on the stack to begin with, since in compiling,
                // they should be compiled first before the bytecode itself is emitted
            default:
                return INTERPRET_ERROR;
        }
    }

    return INTERPRET_SUCCESS;
#undef READ_BYTE
}

InterpretResult interpret(const char* source)
{
    VM vm;

    initVM(&vm);
    compile(source, &vm.chunk);

    // the main run loop
    InterpretResult result = run(&vm);

    freeVM(&vm);

    return result;
}

void freeVM(VM* vm)
{
    freeChunk(&vm->chunk);
}
