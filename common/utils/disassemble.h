#ifndef common_utils_disassemble_h
#define common_utils_disassemble_h

#include <chunk.h>

void disassembleChunk(Chunk* chunk);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
