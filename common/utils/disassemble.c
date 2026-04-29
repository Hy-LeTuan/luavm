#include <disassemble.h>

#include <stdio.h>

void disassembleChunk(Chunk* chunk)
{
    fprintf(stdout, "%-8s%s:\n", "", "Disassemble Chunk");
    for (int offset = 0; offset < chunk->count;)
    {
        fprintf(stdout, "%-8s%s", "", "|");
        offset = disassembleInstruction(chunk, offset);
    }
}

static void message(const char* message)
{
    fprintf(stdout, "%-16s", message);
    return;
}

static int simpleInstruction(const char* code, int offset)
{
    message(code);
    fprintf(stdout, "\n");
    return offset + 1;
}

static int constantInstruction(const char* code, Chunk* chunk, int offset)
{
    message(code);
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d\n", chunk->code[offset + 1]);
    return offset + 2;
}

static int jumpInstruction(const char* code, Chunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];

    message(code);
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d\n", jump);
    return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset)
{
    // [LINE | OFFSET | NAME | OPERANDS]

    fprintf(stdout, "%-8s[%04zu]", "", getOpCodeLine(chunk, offset));
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d", offset);
    fprintf(stdout, "%-8s", "");

    switch ((OPCode)chunk->code[offset])
    {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_MINUS:
            return simpleInstruction("OP_MINUS", offset);
        case OP_MUL:
            return simpleInstruction("OP_MUL", offset);
        case OP_DIV:
            return simpleInstruction("OP_DIV", offset);
        case OP_EXPONENT:
            return simpleInstruction("OP_EXPONENT", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_JOIN:
            return simpleInstruction("OP_CONCAT", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_LOCAL:
            return constantInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return constantInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_ELSE", chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", chunk, offset);
        case OP_FUNCTION:
            return constantInstruction("OP_FUNCTION", chunk, offset);
        case OP_CALL:
            return constantInstruction("OP_CALL", chunk, offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
    }

    return offset;
}
