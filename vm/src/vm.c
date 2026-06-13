#include <vm.h>

#include <disassemble.h>
#include <memory.h>
#include <gc.h>
#include <utils.h>
#include <objtable.h>

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define createevent(e, ename, vm)                                                                  \
    {                                                                                              \
        ObjString* name = copyString(ename, strlen(ename), &vm->strings);                          \
        linkObject(baseobj(name), vm);                                                             \
        vm->events[e] = name;                                                                      \
    }

#define Arith(vm, op, inv, event)                                                                  \
    {                                                                                              \
        Value* b = stackat(vm, 1);                                                                 \
        Value* a = stackat(vm, 2);                                                                 \
        if (IS_NUM(a) && IS_NUM(b))                                                                \
        {                                                                                          \
            reducestack(vm, 2);                                                                    \
            pushStack(inv(AS_NUM(a) op AS_NUM(b)), vm);                                            \
        }                                                                                          \
        else                                                                                       \
            arith(event, vm);                                                                      \
    }

static void resetStack(VM* vm)
{
    vm->stackTop = vm->stack;
}

void initVM(VM* vm)
{
    vm->objectStack = NULL;
    vm->frameCount = 0;
    vm->openUpvalues = NULL;
    vm->cacheSize = 0;
    vm->nvals = 0;

    resetStack(vm);
    initTable(&vm->strings);
    initTable(&vm->globals);

    for (uint8_t i = 0; i < MT_SIZE; i++)
    {
        vm->mts[i] = NULL;
    }

    // setup event keys
    createevent(EVENT_ADD, "__add", vm);
    createevent(EVENT_SUB, "__sub", vm);
    createevent(EVENT_MUL, "__mul", vm);
    createevent(EVENT_DIV, "__div", vm);
    createevent(EVENT_MOD, "__mod", vm);
    createevent(EVENT_POW, "__pow", vm);
    createevent(EVENT_UNM, "__unm", vm);
    createevent(EVENT_CONCAT, "__concat", vm);
    createevent(EVENT_LEN, "__len", vm);
    createevent(EVENT_EQ, "__eq", vm);
    createevent(EVENT_LT, "__lt", vm);
    createevent(EVENT_LE, "__le", vm);
    createevent(EVENT_INDEX, "__index", vm);
    createevent(EVENT_NEWINDEX, "__newindex", vm);
    createevent(EVENT_CALL, "__call", vm);
}

void runtimeError(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->frames[vm->frameCount - 1].ip -
      vm->frames[vm->frameCount - 1].closure->function->chunk.code - 1;
    int line = vm->frames[vm->frameCount - 1].closure->function->chunk.lines[instruction];

    fprintf(stderr, "[line %d] in \n", line);
    resetStack(vm);
}

void linkObject(Object* obj, VM* vm)
{
    if (obj == NULL || obj->next != NULL)
    {
        return;
    }

    obj->next = vm->objectStack;
    vm->objectStack = obj;
}

void pushStack(Value value, VM* vm)
{
    if (vm->stackTop - vm->stack == STACK_MAX)
    {
        runtimeError(vm, "Error, stack limit exceeded.");
        return;
    }

    *vm->stackTop = value;
    vm->stackTop++;
}

static Value* peek(int index, VM* vm)
{
    return &vm->stackTop[-1 - index];
}

Value* popStack(VM* vm)
{
    vm->stackTop--;
    return vm->stackTop;
}

static void setnvals(uint8_t val, VM* vm)
{
    vm->nvals = val;
}

/*
   Adjust the number of expressions in an expression list based on whether a multret expression is
   there. The function also consumes the `nvals` register.
*/
static uint8_t getnexprs(uint8_t nexprs, VM* vm)
{
    uint8_t final = vm->nvals == 0 ? nexprs : nexprs + vm->nvals - 1;
    vm->nvals = 0;

    return final;
}

static Value* getAssignValue(VM* vm)
{
    Value* v = &vm->cache[vm->cacheSize - 1];
    vm->cacheSize--;
    return v;
}

static void concatenate(VM* vm)
{
    ObjString* b = AS_STRING(stackat(vm, 1));
    ObjString* a = AS_STRING(stackat(vm, 2));

    ObjString* result = concatenateString(a, b, &vm->strings);
    linkObject(baseobj(result), vm);

    reducestack(vm, 2);
    pushStack(STRING_VAL(result), vm);
}

/*
   Adjust parameter list to match argument count. For vararg functions, an arg table will be
   generated (if permitted) and all extra arguments are lifted into cache
*/
static uint8_t adjustParams(uint8_t callArity, uint8_t functionArity, FunctionType type, VM* vm)
{
    uint8_t diff = abs(callArity - functionArity);

    if (callArity < functionArity)
    {
        for (uint8_t i = 0; i < diff; i++)
        {
            pushStack(NIL_CONSTANT, vm);
        }
        return 0;
    }
    else if (type == TYPE_FUNCTION)
    {
        if (callArity == functionArity)
        {
            return 0;
        }
        else
        {
            reducestack(vm, diff);
        }
    }
    else
    {
        /*
           callArity == functionArity:
           1. all args have a value
           2. `...` has the last value

           callArity > functionArity:
           1. all args have a value
           2. `...` holds the rest
        */

        // shift 1 to include all argument for `...`
        diff++;

        ObjTable* table = type == TYPE_VARARG ? newObjTable() : NULL;

        vm->cacheSize += diff;

        for (uint8_t i = 0; i < diff; i++)
        {
            Value* v = stackat(vm, i + 1);

            // keep stack order consistent
            setstackat(vm->cache, vm->cacheSize - i - 1, v);

            if (table != NULL)
            {
                tableInsertOrSet(NUM_VAL(diff - i), *v, &table->content);
            }
        }

        reducestack(vm, diff);

        if (table != NULL)
        {
            ObjString* length = copyString("n", 1, &vm->strings);

            tableInsertOrSet(STRING_VAL(length), NUM_VAL(diff), &table->content);
            pushStack(TABLE_VAL(table), vm);
        }
        else
        {
            pushStack(NIL_CONSTANT, vm);
        }
    }

    return diff;
}

/*
   Set up call frame for function call.

   If it's a C call, call the function and write its number of return values to frame.

   If it's a Lua call, set up call frame information.
*/
uint8_t precall(uint8_t nexprs, uint8_t status, VM* vm)
{
    if (vm->frameCount > STACK_MAX)
    {
        runtimeError(vm, "Error, stack overflow.");
        return false;
    }

    uint8_t callArity = getnexprs(nexprs, vm);
    Value* caller = peek(callArity, vm);

    CallFrame* newFrame = nextframe(vm);
    newFrame->slots = stackprev(vm, callArity);
    newFrame->callee = stackprev(vm, callArity + 1);
    newFrame->expected = status;

    if (IS_CLOSURE(caller))
    {
        // load a new frame
        ObjClosure* closure = AS_CLOSURE(caller);

        uint8_t extras =
          adjustParams(callArity, closure->function->arity, closure->function->type, vm);

        newFrame->closure = closure;
        newFrame->ip = closure->function->chunk.code;
        newFrame->info = extras;

        return LUA_CALL;
    }
    else if (IS_NATIVE(caller))
    {
        ObjNativeFunction* native = AS_NATIVE(caller);
        uint8_t nrets = native->function(callArity, vm);

        newFrame->info = nrets;

        return C_CALL;
    }
    else
    {
        runtimeError(vm, "Error, object is not callable.");
        exit(1);
    }
}

static ObjUpvalue* captureUpvalue(Value* location, VM* vm)
{
    ObjUpvalue* upvalue = vm->openUpvalues;
    ObjUpvalue* prev = NULL;

    while (upvalue != NULL && upvalue->location > location)
    {
        prev = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == location)
    {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(location);
    createdUpvalue->next = upvalue;

    if (prev == NULL)
    {
        vm->openUpvalues = createdUpvalue;
    }
    else
    {
        prev->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void closeUpvalues(Value* last, VM* vm)
{
    while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last)
    {
        ObjUpvalue* upvalue = vm->openUpvalues;
        upvalue->storage = *upvalue->location;
        upvalue->location = &upvalue->storage;
        vm->openUpvalues = upvalue->next;
    }
}

/*
   Resolve multret expression and handle null padding if number of values generated is insufficient
*/
static void resolveMultret(uint8_t status, uint8_t maxnrets, Value* vals, VM* vm)
{
    if (IS_MULTRET(status))
    {
        for (uint8_t i = 0; i < maxnrets; i++)
        {
            pushStack(vals[i], vm);
        }

        setnvals(maxnrets, vm);
    }
    else
    {
        uint8_t expected = GET_RET(status);
        uint8_t n = MIN(expected, maxnrets);

        for (uint8_t i = 0; i < n; i++)
        {
            pushStack(vals[i], vm);
            expected--;
        }

        // nil padding
        while (expected)
        {
            pushStack(NIL_CONSTANT, vm);
            expected--;
        }

        setnvals(expected, vm);
    }
}

static void resovleNativeCall(uint8_t retStatus, VM* vm)
{
    CallFrame* frame = currframe(vm);
    uint8_t nrets = frame->info;
    Value* returns = stackprev(vm, nrets);

    setstacktop(vm, frame->callee);
    resolveMultret(retStatus, nrets, returns, vm);
}

Value getEventFromValue(uint8_t t, uint8_t e, VM* vm)
{
    ObjTable* table = vm->mts[t];

    if (table == NULL)
    {
        return NIL_CONSTANT;
    }

    Value eventKey = STRING_VAL(vm->events[e]);
    return otGetRaw(&eventKey, table);
}

static void arith(int event, VM* vm)
{
    // TODO: complete arith operation
    Value* b = stackat(vm, 1);
    Value* a = stackat(vm, 2);
    reducestack(vm, 2);
    pushStack(NIL_CONSTANT, vm);

    runtimeError(vm, "Error, cannot carry out binary operation.");
}

InterpretResult run(VM* vm)
{
    CallFrame* frame = currframe(vm);

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (&frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#ifdef DEBUG_DISASSEMBLE_CHUNK
    disassembleChunk(&frame->closure->function->chunk, "Disassemble Chunk");
#endif

    while (true)
    {
#ifdef DEBUG_STACK_TRACE
        fprintf(stdout, "        STACK: ");
        if (vm->stackTop == vm->stack)
        {
            printf("[EMPTY]");
        }
        else
        {
            for (Value* slot = vm->stack; slot < vm->stackTop; slot++)
            {
                printf("[ ");
                printValue(slot);
                printf(" ]");
            }
        }
        fprintf(stdout, "\n");
        fprintf(stdout, "%s\n", "        -----");
#endif
#ifdef DEBUG_DISASSEMBLE_INSTRUCTION
        disassembleInstruction(&vm->frames[vm->frameCount - 1].closure->function->chunk,
          vm->frames[vm->frameCount - 1].ip -
            vm->frames[vm->frameCount - 1].closure->function->chunk.code);
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value* constant = READ_CONSTANT();

                if (CAN_GC(constant))
                {
                    linkObject(AS_OBJ(constant), vm);
                }

                pushStack(*constant, vm);
                break;
            }
            case OP_LENGTH:
            {
                Value* v = popStack(vm);

                if (IS_TABLE(v))
                {
                    ObjTable* t = AS_TABLE(v);
                    int n = otGetLen(t);
                    pushStack(NUM_VAL(n), vm);
                }
                else if (IS_STRING(v))
                {
                    ObjString* s = AS_STRING(v);
                    pushStack(NUM_VAL(s->length), vm);
                }
                else
                {
                    runtimeError(vm, "Error, cannot get length of current object.");
                }
                break;
            }
            case OP_ADD:
                Arith(vm, +, NUM_VAL, EVENT_ADD);
                break;
            case OP_MINUS:
                Arith(vm, -, NUM_VAL, EVENT_SUB);
                break;
            case OP_MUL:
                Arith(vm, *, NUM_VAL, EVENT_MUL);
                break;
            case OP_DIV:
                Arith(vm, /, NUM_VAL, EVENT_DIV);
                break;
            case OP_EXPONENT:
            {
                Value* b = stackat(vm, 1);
                Value* a = stackat(vm, 2);

                if (IS_NUM(a) && IS_NUM(b))
                {
                    reducestack(vm, 2);
                    Value c = NUM_VAL(pow(AS_NUM(a), AS_NUM(b)));
                    pushStack(c, vm);
                }
                else
                {
                    arith(EVENT_POW, vm);
                }

                break;
            }
            case OP_MODULO:
                if (IS_NUM(stackat(vm, 1)) && IS_NUM(stackat(vm, 2)))
                {
                    LuaNum b = AS_NUM(stackat(vm, 1));
                    LuaNum a = AS_NUM(stackat(vm, 2));
                    reducestack(vm, 2);

                    Value c = NUM_VAL(a - floor(a / b) * b);
                    pushStack(c, vm);
                }
                else
                {
                    arith(EVENT_POW, vm);
                }
                break;
            case OP_JOIN:
            {
                if (IS_STRING(peek(0, vm)) && IS_STRING(peek(1, vm)))
                {
                    concatenate(vm);
                }
                else
                {
                    runtimeError(
                      vm, "RuntimeError: Join operation only permitted between 2 strings.");
                }
                break;
            }
            case OP_LESS:
            {
                Arith(vm, <, BOOL_VAL, EVENT_LE);
                break;
            }
            case OP_GREATER:
            {
                bool standard = IS_NUM(stackat(vm, 1)) && IS_NUM(stackat(vm, 2));

                Arith(vm, >, BOOL_VAL, EVENT_LE);

                if (!standard)
                {
                    Value* top = popStack(vm);
                    if (isFalsey(top))
                    {
                        pushStack(TRUE_VAL(), vm);
                    }
                    else
                    {
                        pushStack(FALSE_VAL(), vm);
                    }
                }
                break;
            }
            case OP_EQUAL:
            {
                Value* b = stackat(vm, 1);
                Value* a = stackat(vm, 2);
                reducestack(vm, 2);
                pushStack(BOOL_VAL(compareValue(a, b)), vm);
                break;
            }
            case OP_NEGATE:
            {
                Value* a = popStack(vm);

                if (IS_NUM(a))
                {
                    pushStack(NUM_VAL(-AS_NUM(a)), vm);
                }
                else
                {
                    runtimeError(vm,
                      "Runtime Error: Cannot negate a value that is not a number or a boolean.");
                }
                break;
            }
            case OP_NOT:
            {
                Value* a = popStack(vm);

                if (IS_BOOL(a))
                {
                    pushStack(BOOL_VAL(!AS_BOOL(a)), vm);
                }
                else
                {
                    runtimeError(vm,
                      "Runtime Error: Cannot use the 'not' operator on a value that is not a "
                      "boolean.");
                }
                break;
            }
            case OP_GET_GLOBAL:
            {
                Value* key = READ_CONSTANT();
                Value v = tableGet(key, &vm->globals);

                // accept a global variable with nil
                pushStack(v, vm);
                break;
            }
            case OP_SET_GLOBAL:
            {
                Value* key = READ_CONSTANT();
                Value* value = getAssignValue(vm);
                tableInsertOrSet(*key, *value, &vm->globals);
                break;
            }
            case OP_GET_LOCAL:
            {
                uint8_t index = READ_BYTE();
                pushStack(frame->slots[index], vm);
                break;
            }
            case OP_SET_LOCAL:
            {
                uint8_t index = READ_BYTE();
                frame->slots[index] = *getAssignValue(vm);
                break;
            }
            case OP_GET_UPVALUE:
            {
                // the index of the upvalue in the function's upvalue array
                uint8_t index = READ_BYTE();
                pushStack(*frame->closure->upvalues[index]->location, vm);
                break;
            }
            case OP_SET_UPVALUE:
            {
                uint8_t index = READ_BYTE();
                *frame->closure->upvalues[index]->location = *getAssignValue(vm);
                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                int offset = READ_SHORT();
                if (isFalsey(peek(0, vm)))
                {
                    frame->ip += offset;
                }
                break;
            }
            case OP_JUMP:
            {
                int offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_LOOP:
            {
                int offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CLOSURE:
            {
                Value* constant = READ_CONSTANT();
                ObjFunction* function = AS_FUNCTION(constant);
                ObjClosure* closure = newClosure(function);

                linkObject((Object*)closure, vm);

                for (int i = 0; i < closure->function->upvalueCount; i++)
                {
                    uint8_t immediate = READ_BYTE();
                    uint8_t index = READ_BYTE();

                    if (immediate)
                    {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index, vm);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }

                pushStack(CLOSURE_VAL(closure), vm);
                break;
            }
            case OP_CALL:
            {
                uint8_t nexprs = READ_BYTE();
                uint8_t retStatus = READ_BYTE();

                if (precall(nexprs, retStatus, vm) == C_CALL)
                {
                    resovleNativeCall(retStatus, vm);
                    frame = prevframe(vm);
                }
                else
                {
                    frame = currframe(vm);
                }
                break;
            }
            case OP_VARARG:
            {
                uint8_t status = READ_BYTE();
                resolveMultret(status, frame->info, vm->cache + vm->cacheSize - frame->info, vm);
                break;
            }
            case OP_SELF:
            {
                // TODO: handle OP_SELF
                break;
            }
            case OP_CONSTRUCT:
            {
                ObjTable* table = newObjTable();

                linkObject(baseobj(table), vm);

                uint8_t fields = READ_BYTE();

                for (int i = 0; i < fields; i++)
                {
                    Value* key = stackat(vm, (fields - i) * 2);
                    Value* value = stackat(vm, (fields - i) * 2 - 1);

                    otSet(*key, *value, table);
                }

                setstacktop(vm, vm->stackTop - fields * 2);

                pushStack(TABLE_VAL(table), vm);
                break;
            }
            case OP_GET_FIELD:
            {
                Value* key = stackat(vm, 1);
                Value* tableVal = stackat(vm, 2);
                reducestack(vm, 2);

                if (IS_TABLE(tableVal))
                {
                    ObjTable* table = AS_TABLE(tableVal);
                    Value val = otGet(key, table);
                    if (!IS_NIL(&val))
                    {
                        pushStack(val, vm);
                        break;
                    }
                }

                // attempt to get value from metatable
                Value indexed = getEventFromValue(vtype(tableVal), EVENT_INDEX, vm);

                if (IS_NIL(&indexed))
                {
                    pushStack(NIL_CONSTANT, vm);
                }
                else if (IS_FUNCTION(&indexed))
                {
                    pushStack(indexed, vm);

                    if (precall(2, MAKE_RET(1), vm) == C_CALL)
                    {
                        resovleNativeCall(MAKE_RET(1), vm);
                        frame = prevframe(vm);
                    }
                    else
                    {
                        frame = currframe(vm);
                    }
                }
                else
                {
                    ObjTable* mt = AS_TABLE(&indexed);
                    Value val = otGet(key, mt);
                    pushStack(val, vm);
                }
                break;
            }
            case OP_SET_FIELD:
            {
                Value* key = stackat(vm, 1);
                Value* tableVal = stackat(vm, 2);
                reducestack(vm, 2);

                if (!IS_TABLE(tableVal))
                {
                    runtimeError(vm, "Error, trying to index into a value that is not a table.");
                    break;
                }

                ObjTable* table = AS_TABLE(tableVal);

                Value* val = getAssignValue(vm);
                otSet(*key, *val, table);
                break;
            }
            case OP_CLOSE_UPVALUE:
            {
                closeUpvalues(vm->stackTop - 1, vm);
                popStack(vm);
                break;
            }
            case OP_CACHE:
            {
                uint8_t n = READ_BYTE();

                if (n > CACHE_MAX)
                {
                    runtimeError(vm, "Error, cache limit exceeded.");
                    break;
                }

                vm->cacheSize += n;

                // reverse the order of the insertion
                for (int i = 0; i < n; i++)
                {
                    Value* v = stackat(vm, i + 1);
                    setstackat(vm->cache, vm->cacheSize - i - 1, v)
                }

                reducestack(vm, n);
                break;
            }
            case OP_POP:
                popStack(vm);
                break;
            case OP_RETURN:
            {
                closeUpvalues(frame->slots, vm);
                vm->cacheSize -= frame->info;
                if (finalframe(vm))
                {
                    return INTERPRET_SUCCESS;
                }

                uint8_t nexprs = READ_BYTE();
                uint8_t nrets = getnexprs(nexprs, vm);
                uint8_t retStatus = frame->expected;

                Value* returns = stackprev(vm, nrets);
                setstacktop(vm, frame->callee);
                resolveMultret(retStatus, nrets, returns, vm);

                frame = prevframe(vm);
                break;
            }
            default:
                return INTERPRET_ERROR;
        }
    }

    return INTERPRET_SUCCESS;
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef EXECUTE_BINARY
}

void freeVM(VM* vm)
{
    resetStack(vm);
    freeTable(&vm->strings);
    freeTable(&vm->globals);
    freeObjects(vm->objectStack);

    for (uint8_t i = 0; i < MT_SIZE; i++)
    {
        freeObject(baseobj(vm->mts[i]));
    }
}
