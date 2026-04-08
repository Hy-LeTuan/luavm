#include <vm.h>
#include <compiler.h>
#include <disassemble.h>

#include <math.h>
#include <stdio.h>

void initVM(VM* vm)
{
    vm->stackTop = &vm->stack[0];
    initChunk(&vm->chunk);
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

static Value pop(VM* vm)
{
    vm->stackTop--;
    return *vm->stackTop;
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
                printValue(slot);
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
                push(READ_CONSTANT(), vm);
                break;
            case OP_ADD:
                EXECUTE_BINARY(+, AS_NUM, NUM_VAL, vm);
                break;
            case OP_MINUS:
                EXECUTE_BINARY(-, AS_NUM, NUM_VAL, vm);
                break;
            case OP_MUL:
                EXECUTE_BINARY(*, AS_NUM, NUM_VAL, vm);
                break;
            case OP_DIV:
                EXECUTE_BINARY(/, AS_NUM, NUM_VAL, vm);
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
            {
                EXECUTE_BINARY(%, (int)AS_NUM, NUM_VAL, vm);
                break;
            }
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
    compile(source, &vm.chunk);

    // the main run loop
    InterpretResult result = run(&vm);

    freeVM(&vm);

    return result;
}

void freeVM(VM* vm)
{
    freeChunk(&vm->chunk);
}
