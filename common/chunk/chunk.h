#ifndef common_chunk_chunk_h
#define common_chunk_chunk_h

#include <table.h>
#include <object.h>
#include <vmstate.h>

typedef enum
{
    OP_CONSTANT,
    OP_NEGATE,
    OP_LENGTH,
    OP_ADD,
    OP_MINUS,
    OP_MUL,
    OP_DIV,
    OP_EXPONENT,
    OP_MODULO,
    OP_JOIN,
    OP_LESS,
    OP_GREATER,
    OP_NOT,
    OP_EQUAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_FUNCTION,
    OP_CLOSURE,
    OP_CALL,
    OP_VARARG,
    OP_SELF,
    OP_CONSTRUCT,
    OP_GET_FIELD,
    OP_SET_FIELD,
    OP_CLOSE_UPVALUE,
    OP_CACHE,
    OP_POP,
    OP_RETURN
} OPCode;

void initChunk(Chunk* c);
void writeChunk(Chunk* c, uint8_t op, size_t l, VM* vm);
size_t addConstant(Chunk* c, Value v, VM* vm);
void freeChunk(Chunk* c, VM* vm);
size_t getOpCodeLine(Chunk* c, int offset);

#endif
