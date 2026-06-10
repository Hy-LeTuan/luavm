#include <file_utils.h>
#include <lapi.h>

int main(int arc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);
    InterpretResult result = execChunkST(source);

    if (result == INTERPRET_ERROR)
    {
        return 1;
    }

    return 0;
}

