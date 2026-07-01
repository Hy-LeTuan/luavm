#ifndef vm_vmdo_h
#define vm_vmdo_h

#include <chunk.h>
#include <value.h>
#include <object.h>
#include <metatable.h>

#define C_CALL 0
#define LUA_CALL 1
#define CALL_ERROR 2

#define IS_MULTRET(status) (status == 0)

#define nextframe(vm) (&G(vm)->frames[G(vm)->frameCount++])
#define currframe(vm) (&G(vm)->frames[G(vm)->frameCount - 1])
#define prevframe(vm) (&G(vm)->frames[(--G(vm)->frameCount) - 1])
#define finalframe(vm) (G(vm)->frameCount - 1 == 0)

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_ERROR
} InterpretResult;

void initVM(GlobalState* g, bool gInit, VM* vm);
InterpretResult run(VM* vm);
void freeVM(VM* vm);
void runtimeError(VM* vm, const char* format, ...);
uint8_t precall(uint8_t nexprs, uint8_t status, VM* vm);

Value* getEventFromTypeMt(const Value* v, uint8_t e, VM* vm);

#endif
