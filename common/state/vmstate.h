#ifndef common_state_vmstate_h
#define common_state_vmstate_h

#include <object.h>
#include <metatable.h>

#define STACK_MAX 512
#define CACHE_MAX 128
#define MT_SIZE 8

#define G(vm) ((vm)->gState)

#define getmtdirect(vm, type) (G(vm)->mts[type])
#define getevent(vm, event) (G(vm)->events[event])

#define stackat(vm, x) ((G(vm)->stackTop - (x)))
#define stackprev(vm, x) (G(vm)->stackTop - (x))
#define reducestack(vm, x) (G(vm)->stackTop -= (x))
#define setstackat(stack, idx, v) (stack[(idx)] = (*v));
#define setstacktop(vm, newSlot) (G(vm)->stackTop = newSlot)

#define unsafe_push(vm, v) (*(G(vm)->stackTop++) = v)
#define unsafe_pop(vm) (G(vm)->stackTop--)

typedef struct
{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
    Value* callee;

    /*
       expected number of return value in status form, which needs conversion before usage
    */
    uint8_t status;

    /*
       the number of extra arguments hidden in a vararg function in Lua call OR the exact return
       values after conversion if it's a C function
    */
    uint8_t info;
} CallFrame;

typedef struct GlobalState
{
    int frameCount;
    size_t bytesAllocated;
    size_t GCthreshold;
    ObjTable* strings;
    ObjTable* libGlobals;
    Value* stackTop;
    Value stack[STACK_MAX];
    CallFrame frames[STACK_MAX];
    ObjTable* mts[MT_SIZE];
    ObjString* events[EVENT_SIZE];
    Object* objectStack;
} GlobalState;

typedef struct VM
{
    GlobalState* gState;
    uint8_t cacheSize;
    /*
       used to keep track of number of values generated from multret expressions
    */
    uint8_t nvals;
    ObjTable* globals;
    ObjUpvalue* openUpvalues;
    Value cache[CACHE_MAX];

} VM;

void pushStack(Value value, VM* vm);
void pushStackPtr(Value* value, VM* vm);
Value* popStack(VM* vm);

#endif
