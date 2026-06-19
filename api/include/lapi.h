#ifndef lapi_h
#define lapi_h

#include <vmstate.h>
#include <vmdo.h>

void setupSingleChunkVM(const char* source, VM* vm);
InterpretResult execChunkST(const char* source);

#endif
