#include "table.h"
#include <vm.h>

#include <compiler.h>
#include <disassemble.h>
#include <memory.h>
#include <gc.h>
#include <objstring.h>

#include <math.h>
#include <stdio.h>

void initVM(VM* vm)
{
    vm->stackTop = &vm->stack[0];
    vm->objectStack = NULL;

    initTable(&vm->strings);
    initTable(&vm->globals);
    initChunk(&vm->chunk);
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

static InterpretResult run(VM* vm)
{
    uint8_t* ip = vm->chunk.code;

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (vm->chunk.constants.values[READ_BYTE()])
#define EXECUTE_BINARY(op, f, f_inv, vm)                                                           \
    do                                                                                             \
    {                                                                                              \
        Value b = pop(vm);                                                                         \
        Value a = pop(vm);                                                                         \
        push(f_inv(f(a) op f(b)), vm);                                                             \
    } while (false)

#ifdef DEBUG_DISASSEMBLE_CHUNK
    disassembleChunk(&vm->chunk);
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
        printf("\n");
        fprintf(stdout, "%s\n", "        -----");
#endif
#ifdef DEBUG_DISASSEMBLE_INSTRUCTION
        disassembleInstruction(&vm->chunk, ip - vm->chunk.code);
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
            case OP_ADD:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(+, AS_NUM, NUM_VAL, vm);
                }
                else if (IS_STRING(peek(0, vm)) && IS_STRING(peek(1, vm)))
                {
                    concatenate(vm);
                }
                break;
            case OP_MINUS:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(-, AS_NUM, NUM_VAL, vm);
                }
                break;
            case OP_MUL:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(*, AS_NUM, NUM_VAL, vm);
                }
                break;
            case OP_DIV:
                if (IS_NUM(peek(0, vm)) && IS_NUM(peek(1, vm)))
                {
                    EXECUTE_BINARY(/, AS_NUM, NUM_VAL, vm);
                }
                break;
            case OP_EXPONENT:
            {
                Value b = pop(vm);
                Value a = pop(vm);
                Value c = NUM_VAL(pow(AS_NUM(a), AS_NUM(b)));
                push(c, vm);
                break;
            }
            case OP_MODULO:
                EXECUTE_BINARY(%, (int)AS_NUM, NUM_VAL, vm);
                break;
            case OP_NEGATE:
            {
                Value a = pop(vm);
                push(NUM_VAL(-AS_NUM(a)), vm);
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
            case OP_POP:
                pop(vm);
                break;
            case OP_RETURN:
                return INTERPRET_SUCCESS;
            default:
                return INTERPRET_ERROR;
        }
    }

    return INTERPRET_SUCCESS;
#undef READ_BYTE
#undef READ_CONSTANT
#undef EXECUTE_BINARY
}

InterpretResult interpret(const char* source)
{
    VM vm;

    initVM(&vm);
    compile(source, &vm.chunk, &vm.strings, &vm.globals);

    // the main run loop
    InterpretResult result = run(&vm);

    freeVM(&vm);

    return result;
}

void freeVM(VM* vm)
{
    freeTable(&vm->strings);
    freeTable(&vm->globals);
    freeChunk(&vm->chunk);
    freeObjects(vm->objectStack);
}
