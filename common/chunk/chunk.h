#ifndef common_chunk_chunk_h
#define common_chunk_chunk_h

#include <stdint.h>
#include <value.h>
#include <table.h>

typedef enum
{
    OP_CONSTANT,
    OP_NEGATE,
    OP_ADD,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_EXPONENT,
    OP_MODULO,
    OP_LESS,
    OP_GREATER,
    OP_NOT,
    OP_EQUAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_FUNCTION,
    OP_CALL,
    OP_POP,
    OP_RETURN
} OPCode;

typedef struct
{
    size_t capacity;
    size_t count;
    uint8_t* code;
    size_t* lines;
    ValueArray constants;
    Table lookup;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t op, size_t line);
size_t addConstant(Chunk* chunk, Value value);
void freeChunk(Chunk* chunk);
size_t getOpCodeLine(Chunk* chunk, int offset);

#endif
