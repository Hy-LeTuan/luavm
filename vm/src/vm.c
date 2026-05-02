#include <vm.h>

#include <compiler.h>
#include <disassemble.h>
#include <memory.h>
#include <gc.h>
#include <objstring.h>

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
        fprintf(stderr, "Error, stack limit exceeded.\n");
        exit(EXIT_FAILURE);
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

static void concatenate(VM* vm)
{
    ObjString* b = AS_STRING(pop(vm));
    ObjString* a = AS_STRING(pop(vm));
    ObjString* result = concatenateString(a, b, &vm->strings);

    linkObject((Object*)result, vm);
    push(OBJ_VAL((Object*)result), vm);
}

static bool call(uint8_t callArity, VM* vm)
{
    Value caller = peek(callArity, vm);

    if (!IS_CLOSURE(caller))
    {
        runtimeError(vm, "Error, cannot call a value that is not a function.");
        return false;
    }
    else if (vm->frameCount > STACK_MAX)
    {
        runtimeError(vm, "Error, stack overflow.");
        return false;
    }

    // load a new frame
    ObjClosure* closure = AS_CLOSURE(caller);
    CallFrame* newFrame = &vm->frames[vm->frameCount];
    vm->frameCount++;

    newFrame->closure = closure;
    newFrame->ip = closure->function->chunk.code;
    newFrame->slots = vm->stackTop - callArity;

    return true;
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

                // For strings, it could be a part of the call syntax if predecessed by a
                // function. Greedily assume it's part of the call syntax without evaluating the
                // expression
                if (IS_STRING(constant))
                {
                    if (vm->stackTop - vm->stack > 2 && IS_FUNCTION(peek(1, vm)))
                    {
                        call(1, vm);
                    }
                }
                else
                {
                    push(constant, vm);
                }

                break;
            }
            case OP_ADD:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(+, AS_NUM, NUM_VAL, vm);
                }
                else if (IS_STRING(peek(0, vm)) && IS_STRING(peek(1, vm)))
                {
                    concatenate(vm);
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
                Value value = peek(0, vm);
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
                frame->slots[index] = peek(0, vm);
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
                *frame->closure->upvalues[index]->location = peek(0, vm);
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
                uint8_t callArity = READ_BYTE();
                if (!call(callArity, vm))
                {
                    return INTERPRET_ERROR;
                }

                frame = &vm->frames[vm->frameCount - 1];
                break;
            }
            case OP_CLOSE_UPVALUE:
            {
                closeUpvalues(vm->stackTop - 1, vm);
                pop(vm);
                break;
            }
            case OP_POP:
                pop(vm);
                break;
            case OP_RETURN:
            {
                Value result = pop(vm);
                closeUpvalues(frame->slots, vm);
                vm->frameCount--;

                if (vm->frameCount == 0)
                {
                    return INTERPRET_SUCCESS;
                }

                // since slots starts at the first argument,
                // we move 1 step back to reach the caller
                vm->stackTop = frame->slots - 1;
                push(result, vm);

                frame = &vm->frames[vm->frameCount - 1];

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

    ObjFunction* function = compile(source, &vm.strings);
    ObjClosure* closure = newClosure(function);

    push(OBJ_VAL((Object*)closure), &vm);
    call(0, &vm);

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
