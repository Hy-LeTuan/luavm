#ifndef lapi_h
#define lapi_h

#include <vmstate.h>
#include <vmdo.h>

void setupSingleChunkVM(const char* source, GlobalState* g, VM* vm);
InterpretResult execChunkST(const char* source);

#endif
