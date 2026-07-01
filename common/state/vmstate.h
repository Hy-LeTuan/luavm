#ifndef common_state_vmstate_h
#define common_state_vmstate_h

#include <object.h>
#include <metatable.h>

#define STACK_MAX 512
#define CACHE_MAX 128
#define MT_SIZE 8

#define stackat(vm, x) ((vm->stackTop - (x)))
#define stackprev(vm, x) (vm->stackTop - (x))
#define reducestack(vm, x) (vm->stackTop -= (x))
#define setstackat(stack, idx, v) (stack[(idx)] = (*v));
#define setstacktop(vm, newSlot) (vm->stackTop = newSlot)

#define unsafe_push(vm, v) (*(vm->stackTop++) = v)
#define unsafe_pop(vm) (vm->stackTop--)

#define G(vm) (vm->gState)

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
    Value stack[STACK_MAX];
    CallFrame frames[STACK_MAX];
    ObjTable* mts[MT_SIZE];
    ObjString* events[EVENT_SIZE];
    Object* objectStack;
} GlobalState;

typedef struct VM
{
    GlobalState* gState;
    Value stack[STACK_MAX];
    Value cache[CACHE_MAX];
    uint8_t cacheSize;
    CallFrame frames[STACK_MAX];
    int frameCount;
    Value* stackTop;
    size_t bytesAllocated;
    size_t GCthreshold;
    Object* objectStack;
    ObjTable* strings;
    ObjTable* globals;
    ObjUpvalue* openUpvalues;

    /* this field is used to keep track of how many values are generated from multret expressions */
    uint8_t nvals;

    ObjTable* mts[MT_SIZE];
    ObjString* events[EVENT_SIZE];
} VM;

void pushStack(Value value, VM* vm);
void pushStackPtr(Value* value, VM* vm);
Value* popStack(VM* vm);

#endif
