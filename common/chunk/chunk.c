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
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk)
{
    FREE(chunk->code, chunk->capacity);
    FREE(chunk->lines, chunk->capacity);

    chunk->capacity = 0;
    chunk->count = 0;

    freeValueArray(&chunk->constants);
}
