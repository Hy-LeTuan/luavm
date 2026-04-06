#include <disassemble.h>

#include <stdio.h>

void disassembleChunk(Chunk* chunk)
{
}

static int simpleInstruction(const char* code, int offset)
{
    printf("'%s'\n", code);
    return offset + 1;
}

static int constantInstruction(Chunk* chunk, int offset)
{
    printf("%s", "'OP_CONSTANT'");
    printf("%-8s", "");
    printf("%04d\n", chunk->code[offset + 1]);
    return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset)
{
    // [LINE | OFFSET | NAME | RELEVANT INFORMATION]

    printf("%-8s[%04zu]", "", getOpCodeLine(chunk, offset));
    printf("%-8s", "");
    printf("%04d", offset);
    printf("%-8s", "");

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
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            return offset;
    }
}
