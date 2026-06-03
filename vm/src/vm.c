#include <vm.h>

#include <compiler.h>
#include <disassemble.h>
#include <memory.h>
#include <gc.h>
#include <objstring.h>
#include <objnativefunction.h>
#include <native_functions.h>
#include <objtable.h>
#include <utils.h>

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

static void runtimeError(VM* vm, const char* format, ...)
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

static void push(Value value, VM* vm)
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

static Value pop(VM* vm)
{
    vm->stackTop--;
    return *vm->stackTop;
}

static void reduceStack(uint8_t amount, VM* vm)
{
    if (vm->stackTop - vm->stack < amount)
    {
        runtimeError(vm, "Internal Error, stack is reduced past the limit.");
    }

    vm->stackTop -= amount;
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

    return pop(vm);
}

static void concatenate(VM* vm)
{
    ObjString* b = AS_STRING(pop(vm));
    ObjString* a = AS_STRING(pop(vm));
    ObjString* result = concatenateString(a, b, &vm->strings);

    linkObject((Object*)result, vm);
    push(OBJ_VAL((Object*)result), vm);
}

/*
   Adjust parameter list to match argument count. For vararg functions, an arg table will be
   generated (if permitted) and all extra arguments are lifted into cache
*/
static uint8_t adjustParamters(uint8_t callArity, uint8_t functionArity, FunctionType type, VM* vm)
{
    uint8_t diff = abs(callArity - functionArity);

    if (callArity < functionArity)
    {
        for (uint8_t i = 0; i < diff; i++)
        {
            push(NIL_CONSTANT, vm);
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
            reduceStack(diff, vm);
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
            Value v = pop(vm);

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

            tableInsertOrSet(OBJ_VAL((Object*)length), NUM_VAL(diff), &table->content);
            push(OBJ_VAL((Object*)table), vm);
        }
        else
        {
            push(NIL_CONSTANT, vm);
        }
    }

    return diff;
}

static bool call(uint8_t nexprs, uint8_t expected, VM* vm)
{
    if (vm->frameCount > STACK_MAX)
    {
        runtimeError(vm, "Error, stack overflow.");
        return false;
    }

    uint8_t callArity = getnexprs(nexprs, vm);
    Value caller = peek(callArity, vm);

    if (IS_CLOSURE(caller))
    {
        // load a new frame
        ObjClosure* closure = AS_CLOSURE(caller);

        uint8_t extras =
          adjustParamters(callArity, closure->function->arity, closure->function->type, vm);

        CallFrame* newFrame = &vm->frames[vm->frameCount];
        vm->frameCount++;

        newFrame->closure = closure;
        newFrame->ip = closure->function->chunk.code;
        newFrame->slots = vm->stackTop - closure->function->arity;
        newFrame->expected = expected;

        if (closure->function->type == TYPE_FUNCTION)
        {
            newFrame->extras = 0;
        }
        else
        {
            newFrame->extras = extras;
        }

        return true;
    }
    else if (IS_NATIVE(caller))
    {
        ObjNativeFunction* native = AS_NATIVE(caller);
        Value result = native->function(vm->stackTop - callArity);
        expected = GET_RET(expected);

        reduceStack(callArity + 1, vm);

        if (expected > 0)
        {
            push(result, vm);
            expected--;
        }

        for (int i = 0; i < expected; i++)
        {
            push(NIL_CONSTANT, vm);
        }

        return true;
    }
    else
    {
        runtimeError(vm, "Error, cannot call a value that is not a function.");
        return false;
    }
}

static void defineNativeFunction(const char* name, int length, NativeFn function, VM* vm)
{
    ObjString* key = copyString(name, length, &vm->strings);
    ObjNativeFunction* native = newNativeFunction(function);

    linkObject((Object*)key, vm);
    linkObject((Object*)native, vm);

    tableInsertOrSet(OBJ_VAL((Object*)key), OBJ_VAL((Object*)native), &vm->globals);
}

static void defineNativeFunctions(VM* vm)
{
    defineNativeFunction("print", 5, print, vm);
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
            push(vals[i], vm);
        }

        setnvals(maxnrets, vm);
    }
    else
    {
        uint8_t expected = GET_RET(status);
        uint8_t n = MIN(expected, maxnrets);

        for (uint8_t i = 0; i < n; i++)
        {
            push(vals[i], vm);
            expected--;
        }

        // nil padding
        while (expected)
        {
            push(NIL_CONSTANT, vm);
            expected--;
        }

        setnvals(expected, vm);
    }
}

InterpretResult run(VM* vm)
{
    CallFrame* frame = &vm->frames[vm->frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define EXECUTE_BINARY(op, f, f_inv, vm)                                                           \
    do                                                                                             \
    {                                                                                              \
        Value b = pop(vm);                                                                         \
        Value a = pop(vm);                                                                         \
        push(f_inv(f(a) op f(b)), vm);                                                             \
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

                push(constant, vm);
                break;
            }
            case OP_LENGTH:
            {
                ObjString* s = AS_STRING(pop(vm));
                push(NUM_VAL(s->length), vm);
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
                Value b = pop(vm);
                Value a = pop(vm);

                if (!IS_NUM(a) || !IS_NUM(b))
                {
                    runtimeError(
                      vm, "RuntimeError: Operation '%c' only permitted between 2 numbers.", '^');
                    break;
                }

                Value c = NUM_VAL(pow(AS_NUM(a), AS_NUM(b)));
                push(c, vm);
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
                Value b = pop(vm);
                Value a = pop(vm);
                push(BOOL_VAL(compareValue(a, b)), vm);
                break;
            }
            case OP_NEGATE:
            {
                Value a = pop(vm);

                if (IS_NUM(a))
                {
                    push(NUM_VAL(-AS_NUM(a)), vm);
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
                Value a = pop(vm);

                if (IS_BOOL(a))
                {
                    push(BOOL_VAL(!AS_BOOL(a)), vm);
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
                push(v, vm);
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
                push(frame->slots[index], vm);
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
                push(*frame->closure->upvalues[index]->location, vm);
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

                push(OBJ_VAL((Object*)closure), vm);
                break;
            }
            case OP_CALL:
            {
                uint8_t nexprs = READ_BYTE();
                uint8_t expected = READ_BYTE();

                if (!call(nexprs, expected, vm))
                {
                    return INTERPRET_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                break;
            }
            case OP_VARARG:
            {
                uint8_t retStatus = READ_BYTE();
                resolveMultret(
                  retStatus, frame->extras, vm->cache + vm->cacheSize - frame->extras, vm);
                break;
            }
            case OP_CONSTRUCT:
            {
                ObjTable* table = newTable();

                linkObject((Object*)table, vm);

                uint8_t fields = READ_BYTE();

                for (int i = 0; i < fields; i++)
                {
                    Value value = pop(vm);
                    Value key = pop(vm);

                    tableInsertOrSet(key, value, &table->content);
                }

                push(OBJ_VAL((Object*)table), vm);
                break;
            }
            case OP_GET_FIELD:
            {
                Value key = pop(vm);
                Value tableVal = pop(vm);
                ObjTable* table = AS_TABLE(tableVal);

                Value val = tableGet(key, &table->content);
                push(val, vm);
                break;
            }
            case OP_SET_FIELD:
            {
                Value key = pop(vm);
                Value tableVal = pop(vm);

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
                pop(vm);
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
                    Value v = pop(vm);
                    vm->cache[vm->cacheSize - i - 1] = v;
                }
                break;
            }
            case OP_POP:
                pop(vm);
                break;
            case OP_RETURN:
            {
                uint8_t nexprs = READ_BYTE();
                uint8_t nrets = getnexprs(nexprs, vm);

                uint8_t retStatus = frame->expected;

                Value* returns = vm->stackTop - nrets;

                closeUpvalues(frame->slots, vm);
                vm->frameCount--;

                if (vm->frameCount == 0)
                {
                    return INTERPRET_SUCCESS;
                }

                vm->cacheSize -= frame->extras;

                /* slots at first argument; move 1 step back to reach the caller */
                vm->stackTop = frame->slots - 1;

                frame = &vm->frames[vm->frameCount - 1];

                resolveMultret(retStatus, nrets, returns, vm);
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

InterpretResult interpret(const char* source)
{
    VM vm;

    initVM(&vm);
    defineNativeFunctions(&vm);

    ObjFunction* function = compile(source, &vm.strings);
    ObjClosure* closure = newClosure(function);

    push(OBJ_VAL((Object*)closure), &vm);
    call(0, 1, &vm);

    // the main run loop
    InterpretResult result = run(&vm);

    freeVM(&vm);

    return result;
}

void freeVM(VM* vm)
{
    resetStack(vm);
    freeTable(&vm->strings);
    freeTable(&vm->globals);
    freeObjects(vm->objectStack);
}
