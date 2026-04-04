#include <chunk.h>
#include <assert.h>
#include <memory.h>

uint8_t opCodeControl[4] = { OP_ADD, OP_MINUS, OP_MUL, OP_DIV };

int main(int argc, char* argv[])
{
    Chunk chunk;
    initChunk(&chunk);

    // write code test
    writeChunk(&chunk, OP_ADD, 0);
    writeChunk(&chunk, OP_MINUS, 0);
    writeChunk(&chunk, OP_MUL, 0);
    writeChunk(&chunk, OP_DIV, 0);

    for (int i = 0; i < chunk.count; i++)
    {
        assert(chunk.ip[i] == opCodeControl[i]);
    }

    // write opcode test
    addConstant(&chunk, NUM_VAL(1));
    addConstant(&chunk, NUM_VAL(2));

    for (int i = 0; i < chunk.constants.count; i++)
    {
        assert(chunk.constants.ptr[i].type == NUMBER);
    }

    freeChunk(&chunk);

    return 0;
}
