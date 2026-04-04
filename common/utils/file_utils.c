#include <stdlib.h>
#include <stdio.h>

char* readSourceFile(const char* path)
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
    unsigned long bytes_read = fread(buffer, sizeof(char), sizeof(char) * buffer_size, file);
    fclose(file);

    if (bytes_read == 0 && buffer_size != 0)
    {
        fprintf(stderr, "Error, cannot successfully read file at '%s'.\n", path);
        exit(1);
    }

    buffer[buffer_size] = '\0';
    return buffer;
}
