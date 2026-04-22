#include <file_utils.h>
#include <vm.h>
#include <compiler.h>

#include <stdio.h>

int main(int arc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);
    VM vm;

    initVM(&vm);
    compile(source, &vm.chunk, &vm.strings, &vm.globals);

    InterpretResult result = run(&vm);

    freeVM(&vm);

    if (result == INTERPRET_ERROR)
    {
        fprintf(stderr, "Error, cannot interpret jump test.\n");
        return 1;
    }

    return 0;
}
