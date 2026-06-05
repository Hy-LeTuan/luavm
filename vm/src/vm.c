#include <vm.h>

#include <disassemble.h>
#include <memory.h>
#include <gc.h>
#include <objstring.h>
#include <objnativefunction.h>
#include <objtable.h>
#include <utils.h>

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>

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

static Value peek(int index, VM* vm)
{
    return vm->stackTop[-1 - index];
}

Value popStack(VM* vm)
{
    vm->stackTop--;
    return *vm->stackTop;
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

static Value getAssignValue(VM* vm)
{
    if (vm->cacheSize > 0)
    {
        Value v = vm->cache[vm->cacheSize - 1];
        vm->cacheSize--;
        return v;
    }

    return popStack(vm);
}

static void concatenate(VM* vm)
{
    ObjString* b = AS_STRING(popStack(vm));
    ObjString* a = AS_STRING(popStack(vm));
    ObjString* result = concatenateString(a, b, &vm->strings);

    linkObject((Object*)result, vm);
    pushStack(OBJ_VAL(result), vm);
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

        ObjTable* table = type == TYPE_VARARG ? newTable() : NULL;

        vm->cacheSize += diff;

        for (uint8_t i = 0; i < diff; i++)
        {
            Value v = popStack(vm);

            // keep stack order consistent
            vm->cache[vm->cacheSize - i - 1] = v;

            if (table != NULL)
            {
                tableInsertOrSet(NUM_VAL(diff - i), v, &table->content);
            }
        }

        if (table != NULL)
        {
            ObjString* length = copyString("n", 1, &vm->strings);

            tableInsertOrSet(OBJ_VAL(length), NUM_VAL(diff), &table->content);
            pushStack(OBJ_VAL(table), vm);
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
    Value caller = peek(callArity, vm);

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

InterpretResult run(VM* vm)
{
    CallFrame* frame = currframe(vm);

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define EXECUTE_BINARY(op, f, f_inv, vm)                                                           \
    do                                                                                             \
    {                                                                                              \
        Value b = popStack(vm);                                                                    \
        Value a = popStack(vm);                                                                    \
        pushStack(f_inv(f(a) op f(b)), vm);                                                        \
    } while (false)

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
                printValue(*slot);
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
                Value constant = READ_CONSTANT();

                if (constant.type == OBJECT)
                {
                    linkObject(AS_OBJ(constant), vm);
                }

                pushStack(constant, vm);
                break;
            }
            case OP_LENGTH:
            {
                ObjString* s = AS_STRING(popStack(vm));
                pushStack(NUM_VAL(s->length), vm);
                break;
            }
            case OP_ADD:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(+, AS_NUM, NUM_VAL, vm);
                }
                else
                {
                    runtimeError(
                      vm, "RuntimeError: Cannot add values that are not numbers or strings.");
                }
                break;
            case OP_MINUS:
                if (!IS_NUM(peek(0, vm)) || !IS_NUM(peek(1, vm)))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '-');
                    break;
                }

                EXECUTE_BINARY(-, AS_NUM, NUM_VAL, vm);
                break;
            case OP_MUL:
                if (!IS_NUM(peek(0, vm)) || !IS_NUM(peek(1, vm)))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '*');
                    break;
                }

                EXECUTE_BINARY(*, AS_NUM, NUM_VAL, vm);
                break;
            case OP_DIV:
                if (!IS_NUM(peek(0, vm)) || !IS_NUM(peek(1, vm)))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '/');
                    break;
                }

                EXECUTE_BINARY(/, AS_NUM, NUM_VAL, vm);
                break;
            case OP_EXPONENT:
            {
                Value b = popStack(vm);
                Value a = popStack(vm);

                if (!IS_NUM(a) || !IS_NUM(b))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '^');
                    break;
                }

                Value c = NUM_VAL(pow(AS_NUM(a), AS_NUM(b)));
                pushStack(c, vm);
                break;
            }
            case OP_MODULO:
                if (!IS_NUM(peek(0, vm)) || !IS_NUM(peek(1, vm)))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '%');
                    break;
                }

                EXECUTE_BINARY(%, (int)AS_NUM, NUM_VAL, vm);
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
                Value b = peek(0, vm);
                Value a = peek(1, vm);

                if (a.type != b.type)
                {
                    runtimeError(vm,
                      "RuntimeError: Cannot compare a value of type %d with type %d.", a.type,
                      b.type);
                }
                else
                {
                    EXECUTE_BINARY(<, AS_NUM, BOOL_VAL, vm);
                }
                break;
            }
            case OP_GREATER:
            {
                Value b = peek(0, vm);
                Value a = peek(1, vm);

                if (a.type != b.type)
                {
                    runtimeError(vm,
                      "RuntimeError: Cannot compare a value of type %d with type %d.", a.type,
                      b.type);
                }
                else
                {
                    EXECUTE_BINARY(>, AS_NUM, BOOL_VAL, vm);
                }
                break;
            }
            case OP_EQUAL:
            {
                Value b = popStack(vm);
                Value a = popStack(vm);
                pushStack(BOOL_VAL(compareValue(a, b)), vm);
                break;
            }
            case OP_NEGATE:
            {
                Value a = popStack(vm);

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
                Value a = popStack(vm);

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
                Value key = READ_CONSTANT();
                Value v = tableGet(key, &vm->globals);

                // accept a global variable with nil
                pushStack(v, vm);
                break;
            }
            case OP_SET_GLOBAL:
            {
                Value key = READ_CONSTANT();
                Value value = getAssignValue(vm);
                tableInsertOrSet(key, value, &vm->globals);
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
                frame->slots[index] = getAssignValue(vm);
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
                *frame->closure->upvalues[index]->location = getAssignValue(vm);
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
                Value constant = READ_CONSTANT();
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

                pushStack(OBJ_VAL(closure), vm);
                break;
            }
            case OP_CALL:
            {
                uint8_t nexprs = READ_BYTE();
                uint8_t retStatus = READ_BYTE();

                uint8_t callStatus;
                if ((callStatus = precall(nexprs, retStatus, vm)) == C_CALL)
                {
                    frame = currframe(vm);
                    uint8_t nrets = frame->info;
                    Value* returns = stackprev(vm, nrets);

                    setstacktop(vm, frame->callee);
                    resolveMultret(retStatus, nrets, returns, vm);

                    frame = prevframe(vm);
                }
                else if (callStatus == LUA_CALL)
                {
                    frame = currframe(vm);
                }
                else
                {
                    return INTERPRET_ERROR;
                }
                break;
            }
            case OP_VARARG:
            {
                uint8_t status = READ_BYTE();
                resolveMultret(status, frame->info, vm->cache + vm->cacheSize - frame->info, vm);
                break;
            }
            case OP_CONSTRUCT:
            {
                ObjTable* table = newTable();

                linkObject((Object*)table, vm);

                uint8_t fields = READ_BYTE();

                for (int i = 0; i < fields; i++)
                {
                    Value value = popStack(vm);
                    Value key = popStack(vm);

                    tableInsertOrSet(key, value, &table->content);
                }

                pushStack(OBJ_VAL(table), vm);
                break;
            }
            case OP_GET_FIELD:
            {
                Value key = popStack(vm);
                Value tableVal = popStack(vm);
                ObjTable* table = AS_TABLE(tableVal);

                Value val = tableGet(key, &table->content);
                pushStack(val, vm);
                break;
            }
            case OP_SET_FIELD:
            {
                Value key = popStack(vm);
                Value tableVal = popStack(vm);

                if (!IS_TABLE(tableVal))
                {
                    runtimeError(vm, "Error, trying to index into a value that is not a table.");
                    break;
                }

                ObjTable* table = AS_TABLE(tableVal);

                Value val = getAssignValue(vm);
                tableInsertOrSet(key, val, &table->content);
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
                    Value v = popStack(vm);
                    vm->cache[vm->cacheSize - i - 1] = v;
                }
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
}
