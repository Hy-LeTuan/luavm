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

static int constantInstruction(Chunk* chunk, int offset)
{
    fprintf(stdout, "%s", "'OP_CONSTANT'");
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d\n", chunk->code[offset + 1]);
    return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset)
{
    // [LINE | OFFSET | NAME | RELEVANT INFORMATION]

    fprintf(stdout, "%-8s[%04zu]", "", getOpCodeLine(chunk, offset));
    fprintf(stdout, "%-8s", "");
    fprintf(stdout, "%04d", offset);
    fprintf(stdout, "%-8s", "");

    switch (chunk->code[offset])
    {
        case OP_CONSTANT:
            return constantInstruction(chunk, offset);
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
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            return offset;
    }
}
