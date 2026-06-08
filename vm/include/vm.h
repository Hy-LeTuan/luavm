#ifndef vm_vm_h
#define vm_vm_h

#include <chunk.h>
#include <value.h>
#include <object.h>
#include <metatable.h>

#define MT_SIZE 8

#define STACK_MAX 256
#define CACHE_MAX 128

#define C_CALL 0
#define LUA_CALL 1
#define CALL_ERROR 2

#define IS_MULTRET(status) (status == 0)

#define stackprev(vm, x) (vm->stackTop - (x))
#define reducestack(vm, x) (vm->stackTop -= (x))
#define setstacktop(vm, newSlot) (vm->stackTop = newSlot)

#define nextframe(vm) (&vm->frames[vm->frameCount++])
#define currframe(vm) (&vm->frames[vm->frameCount - 1])
#define prevframe(vm) (&vm->frames[(--vm->frameCount) - 1])
#define finalframe(vm) (vm->frameCount - 1 == 0)

#define getmtdirect(vm, type) (vm->mts[type])

typedef enum
{
    INTERPRET_SUCCESS,
    INTERPRET_ERROR
} InterpretResult;

typedef struct
{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
    Value* callee;

    /* expected number of return value */
    uint8_t expected;

    /* the number of extra arguments hidden in a vararg function in Lua call OR the number of return
     * values from a C function */
    uint8_t info;
} CallFrame;

typedef struct VM
{
    Value stack[STACK_MAX];
    Value cache[CACHE_MAX];
    uint8_t cacheSize;
    CallFrame frames[STACK_MAX];
    int frameCount;
    Value* stackTop;
    Object* objectStack;
    Table strings;
    Table globals;
    ObjUpvalue* openUpvalues;

    /* this field is used to keep track of how many values are generated from multret expressions */
    uint8_t nvals;

    ObjTable* mts[MT_SIZE];
    ObjString* events[EVENT_SIZE];
} VM;

void initVM(VM* vm);
InterpretResult run(VM* vm);
void linkObject(Object* obj, VM* vm);
void freeVM(VM* vm);
void runtimeError(VM* vm, const char* format, ...);
uint8_t precall(uint8_t nexprs, uint8_t status, VM* vm);
void pushStack(Value value, VM* vm);
Value popStack(VM* vm);

Value getEventFromValue(uint8_t t, uint8_t e, VM* vm);
void setEventFromValue(Value v, uint8_t t, uint8_t e, VM* vm);

#endif
