#include <file_utils.h>
#include <vm.h>
#include <compiler.h>

#include <stdio.h>

static Value peek(int index, VM* vm)
{
    return vm->stackTop[-1 - index];
}

int main(int arc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);
    VM vm;

    initVM(&vm);
    compile(source, &vm.chunk, &vm.strings, &vm.globals);

    // the main run loop
    InterpretResult result = run(&vm);

    freeVM(&vm);

    if (result == INTERPRET_ERROR)
    {
        fprintf(stderr, "Error, cannot interpret the given code.\n");
    }
    return 0;
}
