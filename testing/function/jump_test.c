#include <file_utils.h>
#include <vm.h>
#include <compiler.h>

#include <stdio.h>

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
