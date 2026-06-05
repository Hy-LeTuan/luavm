#include <vm.h>
#include <file_utils.h>
#include <lapi.h>

#include <stdlib.h>
#include <stdio.h>

static uint8_t execute(const char* source)
{
    VM vm;
    setupSingleChunkVM(source, &vm);
    InterpretResult result = run(&vm);
    freeVM(&vm);
    if (result == INTERPRET_SUCCESS)
    {
        return 0;
    }
    else
    {
        if (result == INTERPRET_ERROR)
        {
            fprintf(stderr, "Error, cannot interpret the given code.\n");
        }
        return 1;
    }
}

static uint8_t runFile(const char* path)
{
    const char* source = readSourceFile(path);
    uint8_t status = execute(source);
    free((void*)source);

    return status;
}

static uint8_t repl()
{
    char buffer[1024];
    uint8_t status;

    while (true)
    {
        printf("> ");

        if (!fgets(buffer, 1024, stdin))
        {
            fprintf(stderr, "Error, cannot parse line.\n");
            break;
        }

        uint8_t status = execute(buffer);

        if (status != 0)
        {
            fprintf(stderr, "Error in repl.\n");
            break;
        }
    }

    return status;
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("Starting luavm repl\n");
        return repl();
    }
    else
    {
        return runFile(argv[1]);
    }
}
