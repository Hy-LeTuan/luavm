#include "value.h"
#include <chunk.h>
#include <memory.h>

#include <stdio.h>

void initChunk(Chunk* chunk)
{
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    chunk->lines = NULL;

    initValueArray(&chunk->constants);
    initTable(&chunk->lookup);
}

void writeChunk(Chunk* chunk, uint8_t op, size_t line)
{
    if (chunk->count + 1 >= chunk->capacity)
    {
        int newCapacity = GROW_SIZE(chunk->capacity);

        // grow op
        chunk->code = REALLOCATE(chunk->code, chunk->capacity, newCapacity, uint8_t);

        // grow line
        chunk->lines = REALLOCATE(chunk->lines, chunk->capacity, newCapacity, size_t);

        chunk->capacity = newCapacity;
    }

    chunk->code[chunk->count] = op;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

size_t getOpCodeLine(Chunk* chunk, int offset)
{
    return chunk->lines[offset];
}

size_t addConstant(Chunk* chunk, Value value)
{
    Value index = tableGet(value, &chunk->lookup);

    if (IS_NIL(index))
    {
        writeValueArray(&chunk->constants, value);
        tableInsertOrSet(value, NUM_VAL((double)chunk->constants.count - 1), &chunk->lookup);
        return chunk->constants.count - 1;
    }

    return (size_t)AS_NUM(index);
}

void freeChunk(Chunk* chunk)
{
    FREE_ARRAY(chunk->code, chunk->capacity, uint8_t);
    FREE_ARRAY(chunk->lines, chunk->capacity, size_t);

    chunk->capacity = 0;
    chunk->count = 0;

    freeValueArray(&chunk->constants);
    freeTable(&chunk->lookup);
}
