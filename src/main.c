#include <vm.h>
#include <file_utils.h>

#include <stdlib.h>
#include <stdio.h>

static void runFile(const char* path)
{
    const char* source = readSourceFile(path);
    interpret(source);
    free((void*)source);
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

        interpret(buffer);
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
