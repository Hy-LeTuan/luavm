#ifndef lapi_h
#define lapi_h

#include <vm.h>

void setupSingleChunkVM(const char* source, VM* vm);
InterpretResult execChunkST(const char* source);

#endif
