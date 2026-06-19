#include <chunk.h>
#include <assert.h>
#include <memory.h>

#include <stdio.h>

uint8_t opCodeControl[4] = { [0] = OP_ADD, [1] = OP_MINUS, [2] = OP_MUL, [3] = OP_DIV };

int main(int argc, char* argv[])
{
    Chunk chunk;
    initChunk(&chunk);

    // write code test
    writeChunk(&chunk, OP_ADD, 0, NULL);
    writeChunk(&chunk, OP_MINUS, 0, NULL);
    writeChunk(&chunk, OP_MUL, 0, NULL);
    writeChunk(&chunk, OP_DIV, 0, NULL);

    for (int i = 0; i < chunk.count; i++)
    {
        assert(chunk.code[i] == opCodeControl[i]);
    }

    fprintf(stdout, "Direct write chunk test passed successfully.\n");

    // add num
    addConstant(&chunk, NUM_VAL(1), NULL);
    addConstant(&chunk, NUM_VAL(1), NULL);

    addConstant(&chunk, NUM_VAL(2), NULL);
    addConstant(&chunk, NUM_VAL(2), NULL);

    addConstant(&chunk, NUM_VAL(3), NULL);
    addConstant(&chunk, NUM_VAL(3), NULL);

    addConstant(&chunk, NUM_VAL(4), NULL);
    addConstant(&chunk, NUM_VAL(4), NULL);

    for (int i = 0; i < chunk.ccount; i++)
    {
        assert(chunk.constants[i].type == NUMBER);
    }

    fprintf(stdout, "Adding constant test passed successfully.\n");

    freeChunk(&chunk, NULL);

    return 0;
}
