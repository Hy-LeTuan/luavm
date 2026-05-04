#include <file_utils.h>
#include <vm.h>
#include <compiler.h>

static Value peek(int index, VM* vm)
{
    return vm->stackTop[-1 - index];
}

int main(int arc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);
    InterpretResult result = interpret(source);

    if (result == INTERPRET_ERROR)
    {
        return 1;
    }

    return 0;
}
