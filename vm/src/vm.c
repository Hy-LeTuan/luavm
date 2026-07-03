#include <vmdo.h>

#include <disassemble.h>
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
        ObjString* name = copyString(ename, strlen(ename), vm);                                    \
        G(vm)->events[e] = name;                                                                   \
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

static void resetStack(GlobalState* g)
{
    g->stackTop = g->stack;
}

void initGlobal(GlobalState* g, VM* vm)
{
    g->objectStack = NULL;
    g->frameCount = 0;
    g->bytesAllocated = 0;
    g->GCthreshold = 1024 * 1024;

    resetStack(g);

    /* set null for GC */
    g->strings = NULL;
    g->libGlobals = NULL;

    for (uint8_t i = 0; i < MT_SIZE; i++)
    {
        g->mts[i] = NULL;
    }
    for (uint8_t i = 0; i < EVENT_SIZE; i++)
    {
        g->events[i] = NULL;
    }

    /* init with allocation */
    g->strings = newObjTable(vm);
    g->libGlobals = newObjTable(vm);

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

void initVM(GlobalState* g, bool gInit, VM* vm)
{
    vm->openUpvalues = NULL;
    vm->globals = NULL;
    vm->cacheSize = 0;
    vm->nvals = 0;

    vm->gState = g;
    if (!gInit)
    {
        initGlobal(g, vm);
    }

    /* init with allocation */
    vm->globals = newObjTable(vm);
}

void runtimeError(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = G(vm)->frames[G(vm)->frameCount - 1].ip -
      G(vm)->frames[G(vm)->frameCount - 1].closure->function->chunk.code - 1;
    int line = G(vm)->frames[G(vm)->frameCount - 1].closure->function->chunk.lines[instruction];

    fprintf(stderr, "[line %d] in \n", line);
    resetStack(G(vm));
}

void pushStack(Value value, VM* vm)
{
    if (G(vm)->stackTop - G(vm)->stack == STACK_MAX)
    {
        runtimeError(vm, "Error, stack limit exceeded.");
        return;
    }

    *G(vm)->stackTop = value;
    G(vm)->stackTop++;
}

void pushStackPtr(Value* value, VM* vm)
{
    if (value == NULL)
    {
        pushStack(NIL_CONSTANT, vm);
    }
    else
    {
        pushStack(*value, vm);
    }
}

static Value* peek(int index, VM* vm)
{
    return &G(vm)->stackTop[-1 - index];
}

Value* popStack(VM* vm)
{
    G(vm)->stackTop--;
    return G(vm)->stackTop;
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
    Value* v = &vm->cache[--vm->cacheSize];
    return v;
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

        ObjTable* table = type == TYPE_VARARG ? newObjTable(vm) : NULL;
        lock_object(baseobj(table));

        int cacheSize = vm->cacheSize + diff;

        for (uint8_t i = 0; i < diff; i++)
        {
            Value* v = stackat(vm, i + 1);

            // keep stack order consistent
            setstackat(vm->cache, cacheSize - i - 1, v);

            if (table != NULL)
            {
                tableInsertOrSet(NUM_VAL(diff - i), *v, &table->content, vm);
            }
        }

        vm->cacheSize = cacheSize;

        release_object(baseobj(table));
        reducestack(vm, diff);

        if (table != NULL)
        {
            pushStack(TABLE_VAL(table), vm);
            ObjString* length = copyString("n", 1, vm);
            tableInsertOrSet(STRING_VAL(length), NUM_VAL(diff), &table->content, vm);
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
    if (G(vm)->frameCount > STACK_MAX)
    {
        runtimeError(vm, "Error, stack overflow.");
        return false;
    }

    uint8_t callArity = getnexprs(nexprs, vm);
    Value* caller = peek(callArity, vm);

    CallFrame* newFrame = nextframe(vm);
    newFrame->slots = stackprev(vm, callArity);
    newFrame->callee = stackprev(vm, callArity + 1);
    newFrame->status = status;

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

    ObjUpvalue* createdUpvalue = newUpvalue(location, vm);
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

Value* getEventFromTypeMt(const Value* v, uint8_t e, VM* vm)
{
    ObjTable* table = getmtdirect(vm, vtype(v));

    if (table == NULL)
    {
        return NULL;
    }

    Value eventKey = STRING_VAL(G(vm)->events[e]);
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
#define READ_CONSTANT() (&frame->closure->function->chunk.constants[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#ifdef DEBUG_DISASSEMBLE_CHUNK
    disassembleChunk(&frame->closure->function->chunk, "Disassemble Chunk");
#endif

    while (true)
    {
#ifdef DEBUG_STACK_TRACE
        fprintf(stdout, "        STACK: ");
        if (G(vm)->stackTop == G(vm)->stack)
        {
            printf("[EMPTY]");
        }
        else
        {
            for (Value* slot = G(vm)->stack; slot < G(vm)->stackTop; slot++)
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
        disassembleInstruction(&G(vm)->frames[G(vm)->frameCount - 1].closure->function->chunk,
          G(vm)->frames[G(vm)->frameCount - 1].ip -
            G(vm)->frames[G(vm)->frameCount - 1].closure->function->chunk.code);
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value* constant = READ_CONSTANT();
                pushStackPtr(constant, vm);
                break;
            }
            case OP_LENGTH:
            {
                Value* v = stackat(vm, 1);
                int n = 0;

                if (IS_TABLE(v))
                {
                    ObjTable* t = AS_TABLE(v);
                    n = otGetLen(t);
                }
                else if (IS_STRING(v))
                {
                    ObjString* s = AS_STRING(v);
                    n = s->length;
                }
                else
                {
                    runtimeError(vm, "Error, cannot get length of current object.");
                }

                popStack(vm);
                pushStack(NUM_VAL(n), vm);
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
                    Value c = NUM_VAL(pow(AS_NUM(a), AS_NUM(b)));
                    reducestack(vm, 2);
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

                    Value c = NUM_VAL(MOD(a, b));

                    reducestack(vm, 2);
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
                    ObjString* b = AS_STRING(stackat(vm, 1));
                    ObjString* a = AS_STRING(stackat(vm, 2));

                    ObjString* result = concatenateString(a, b, vm);

                    reducestack(vm, 2);
                    pushStack(STRING_VAL(result), vm);
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
                        pushStack(TRUE_CONSTANT, vm);
                    }
                    else
                    {
                        pushStack(FALSE_CONSTANT, vm);
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
                Value* v = tableGet(key, TABLE(vm->globals));

                // if global in current module not found, try library globals
                if (IS_NIL(v))
                {
                    v = tableGet(key, TABLE(G(vm)->libGlobals));
                }

                // accept a global variable with nil
                pushStackPtr(v, vm);
                break;
            }
            case OP_SET_GLOBAL:
            {
                Value* key = READ_CONSTANT();
                Value* value = getAssignValue(vm);

                lock_value(value);

                /*
                   assigning to the library global
                */
                if (IS_NIL(tableGet(key, TABLE(G(vm)->libGlobals))))
                {
                    tableInsertOrSet(*key, *value, TABLE(vm->globals), vm);
                }
                else
                {
                    tableInsertOrSet(*key, *value, TABLE(G(vm)->libGlobals), vm);
                }

                release_value(value);
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
                pushStackPtr(frame->closure->upvalues[index]->location, vm);
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

                ObjClosure* closure = newClosure(function, vm);
                pushStack(CLOSURE_VAL(closure), vm);

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
                    InterpretResult subRes = run(vm);

                    if (subRes == INTERPRET_ERROR)
                    {
                        return INTERPRET_ERROR;
                    }

                    // reload the current frame after return
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
                ObjTable* t = newObjTable(vm);

                uint8_t fields = READ_BYTE();
                lock_object(baseobj(t));

                for (int i = 0; i < fields; i++)
                {
                    Value* key = stackat(vm, (fields - i) * 2);
                    Value* value = stackat(vm, (fields - i) * 2 - 1);

                    otSet(*key, *value, t, vm);
                }

                release_object(baseobj(t));
                setstacktop(vm, G(vm)->stackTop - fields * 2);

                pushStack(TABLE_VAL(t), vm);
                break;
            }
            case OP_GET_FIELD:
            {
                Value* key = stackat(vm, 1);
                Value* tableVal = stackat(vm, 2);

                if (IS_TABLE(tableVal))
                {
                    ObjTable* table = AS_TABLE(tableVal);
                    Value* val = otGet(key, table);
                    if (!IS_NIL(val))
                    {
                        reducestack(vm, 2);
                        pushStackPtr(val, vm);
                        break;
                    }
                }

                // attempt to get value from metatable
                Value* indexed = getEventFromTypeMt(tableVal, EVENT_INDEX, vm);

                if (IS_NIL(indexed))
                {
                    reducestack(vm, 2);
                    pushStack(NIL_CONSTANT, vm);
                }
                else if (IS_FUNCTION(indexed))
                {
                    reducestack(vm, 2);
                    pushStackPtr(indexed, vm);

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
                    ObjTable* mt = AS_TABLE(indexed);
                    Value* val = otGet(key, mt);
                    reducestack(vm, 2);
                    pushStackPtr(val, vm);
                }
                break;
            }
            case OP_SET_FIELD:
            {
                Value* key = stackat(vm, 1);
                Value* tableVal = stackat(vm, 2);

                if (!IS_TABLE(tableVal))
                {
                    runtimeError(vm, "Error, trying to index into a value that is not a table.");
                    break;
                }

                ObjTable* table = AS_TABLE(tableVal);

                Value* val = getAssignValue(vm);

                lock_value(val);
                otSet(*key, *val, table, vm);
                release_value(val);

                reducestack(vm, 2);
                break;
            }
            case OP_CLOSE_UPVALUE:
            {
                closeUpvalues(G(vm)->stackTop - 1, vm);
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

                int cacheSize = vm->cacheSize + n;

                // reverse the order of the insertion
                for (int i = 0; i < n; i++)
                {
                    Value* v = stackat(vm, i + 1);
                    setstackat(vm->cache, cacheSize - i - 1, v)
                }

                reducestack(vm, n);
                vm->cacheSize = cacheSize;
                break;
            }
            case OP_POP:
                popStack(vm);
                break;
            case OP_RETURN:
            {
                closeUpvalues(frame->slots, vm);

                /* the only case where we cache is used */
                if (frame->closure->function->type == TYPE_VARARG)
                {
                    vm->cacheSize -= frame->info;
                }

                if (!finalframe(vm))
                {
                    uint8_t nexprs = READ_BYTE();
                    uint8_t nrets = getnexprs(nexprs, vm);
                    uint8_t retStatus = frame->status;

                    Value* returns = stackprev(vm, nrets);
                    setstacktop(vm, frame->callee);
                    resolveMultret(retStatus, nrets, returns, vm);

                    /* this also reduces frame count */
                    frame = prevframe(vm);
                }

                return INTERPRET_SUCCESS;
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
    // freeTable(&vm->globals, vm);
    // freeTable(&vm->strings, vm);

    /*
       free all live objects that hasn't been collected by GC
    */
    freeObjects(G(vm)->objectStack, vm);
}
