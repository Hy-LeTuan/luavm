#include <vm.h>

#include <stdlib.h>
#include <stdio.h>

static char* readFile(const char* path)
{
    char* buffer = NULL;
    FILE* file = fopen(path, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Error, cannot open file at '%s'.\n", path);
        exit(1);
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Error, cannot determine size of file at '%s'.\n", path);
        exit(1);
    }

    long buffer_size = ftell(file);

    if (buffer_size != 0)
    {
        buffer = malloc((buffer_size + 1) * sizeof(char));
    }

    if (buffer == NULL)
    {
        fprintf(stderr, "Error, cannot load file at '%s'.\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_SET);
    fread(buffer, sizeof(char), sizeof(char) * buffer_size, file);

    buffer[buffer_size] = '\0';

    return buffer;
}

static void runFile(const char* path)
{
    const char* source = readFile(path);
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
