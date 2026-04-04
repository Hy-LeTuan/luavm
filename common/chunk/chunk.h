#ifndef common_chunk_chunk_h
#define common_chunk_chunk_h

#include <stdint.h>
#include <value.h>

typedef enum
{
    OP_CONSTANT,
    OP_ADD,
    OP_MINUS,
    OP_MUL,
    OP_DIV
} OPCode;

typedef struct
{
    size_t capacity;
    size_t count;
    uint8_t* code;
    size_t* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t op, size_t line);
size_t addConstant(Chunk* chunk, Value value);
void freeChunk(Chunk* chunk);

#endif
