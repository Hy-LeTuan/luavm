#include <chunk.h>
#include <assert.h>
#include <memory.h>
#include <table.h>
#include <vmstate.h>
#include <vmdo.h>

#include <stdio.h>

uint8_t opCodeControl[4] = { OP_ADD, OP_MINUS, OP_MUL, OP_DIV };

int main(int argc, char* argv[])
{
    Chunk chunk;
    Table table;
    VM vm;

    initVM(&vm);
    initChunk(&chunk);
    initTable(&table);

    // write code test
    writeChunk(&chunk, OP_ADD, 0, &vm);
    writeChunk(&chunk, OP_MINUS, 0, &vm);
    writeChunk(&chunk, OP_MUL, 0, &vm);
    writeChunk(&chunk, OP_DIV, 0, &vm);

    for (int i = 0; i < chunk.count; i++)
    {
        assert(chunk.code[i] == opCodeControl[i]);
    }

    fprintf(stdout, "Direct write chunk test passed successfully.\n");

    // add num
    addConstant(&chunk, NUM_VAL(1), &table, &vm);
    addConstant(&chunk, NUM_VAL(1), &table, &vm);

    addConstant(&chunk, NUM_VAL(2), &table, &vm);
    addConstant(&chunk, NUM_VAL(2), &table, &vm);

    addConstant(&chunk, NUM_VAL(3), &table, &vm);
    addConstant(&chunk, NUM_VAL(3), &table, &vm);

    addConstant(&chunk, NUM_VAL(4), &table, &vm);
    addConstant(&chunk, NUM_VAL(4), &table, &vm);

    for (int i = 0; i < chunk.ccount; i++)
    {
        assert(chunk.constants[i].type == NUMBER);
    }

    fprintf(stdout, "Adding constant test passed successfully.\n");

    freeChunk(&chunk, &vm);
    freeTable(&table, &vm);

    return 0;
}
