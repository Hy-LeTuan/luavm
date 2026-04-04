#include <vm.h>
#include <file_utils.h>

#include <stdlib.h>
#include <stdio.h>

static void handleResult(InterpretResult result)
{
    if (result == INTERPRET_ERROR)
    {
        fprintf(stderr, "Error, cannot interpret the given code.\n");
    }
}

static void runFile(const char* path)
{
    const char* source = readSourceFile(path);
    InterpretResult result = interpret(source);
    free((void*)source);

    handleResult(result);
}

static void repl()
{
    char buffer[1024];

    while (true)
    {
        printf("> ");

        if (!fgets(buffer, 1024, stdin))
        {
            fprintf(stderr, "Error, cannot parse line.\n");
            break;
        }

        InterpretResult result = interpret(buffer);
        handleResult(result);
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("Starting luavm repl\n");
        repl();
    }
    else
    {
        runFile(argv[1]);
    }
}
