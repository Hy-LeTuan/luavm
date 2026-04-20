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

static int simpleInstruction(const char* code, int offset)
{
    fprintf(stdout, "'%s'\n", code);
    return offset + 1;
}

static int constantInstruction(const char* message, Chunk* chunk, int offset)
{
    fprintf(stdout, "'%s'", message);
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d\n", chunk->code[offset + 1]);
    return offset + 2;
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
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
    }

    return offset;
}
