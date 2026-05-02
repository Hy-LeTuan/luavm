#ifndef common_utils_disassemble_h
#define common_utils_disassemble_h

#include <chunk.h>

void disassembleChunk(Chunk* chunk, const char* chunkName);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
