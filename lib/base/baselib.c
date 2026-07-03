#include <baselib.h>

#include <object.h>
#include <objtable.h>
#include <vmdo.h>
#include <file_utils.h>
#include <compiler.h>
#include <gc.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef DEBUG_REQUIRE
#define DEFAULT_PATTERN_COUNT 5
#else
#define DEFAULT_PATTERN_COUNT 1
#endif

/* default patterns */
Field DefaultPatterns[DEFAULT_PATTERN_COUNT] = { { .s = "./?.lua", .len = 7 },
#ifdef DEBUG_REQUIRE
    { .s = "../?.lua", .len = 8 }, { .s = "../../?.lua", .len = 11 },
    { .s = "../testing/data/benchmarks/?.lua", .len = 32 },
    { .s = "../../testing/data/benchmarks/?.lua", .len = 35 }
#endif
};

static void printAux(Value* arg)
{
    if (IS_NUM(arg))
    {
        float num = AS_NUM(arg);

        if (floor(num) == num)
        {
            fprintf(stdout, "%d", (int)num);
        }
        else
        {
            fprintf(stdout, "%.2f", AS_NUM(arg));
        }
    }
    else if (IS_BOOL(arg))
    {
        fprintf(stdout, "%s", AS_BOOL(arg) ? "true" : "false");
    }
    else if (IS_NIL(arg))
    {
        fprintf(stdout, "nil");
    }
    else if (IS_STRING(arg))
    {
        ObjString* string = v2obj(ObjString, arg);
        fprintf(stdout, "%s", string->chars);
    }
    else if (IS_TABLE(arg))
    {
        fprintf(stdout, "table");
    }
    else if (IS_FUNCTION(arg))
    {
        fprintf(stdout, "function");
    }
    else if (IS_CLOSURE(arg))
    {
        fprintf(stdout, "closure");
    }
    else if (IS_NATIVE(arg))
    {
        fprintf(stdout, "native_fn");
    }
    else if (IS_UPVALUE(arg))
    {
        fprintf(stdout, "upvalue");
    }
}

uint8_t lib_print(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    for (uint8_t i = 0; i < narg; i++)
    {
        printAux(&frame->slots[i]);

        if (i != narg - 1)
        {
            printf("\t");
        }
    }

    printf("\n");

    return 0;
}

static uint8_t ipairsAux(uint8_t arg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    Value* state = &frame->slots[0];
    Value* var = &frame->slots[1];

    if (!IS_TABLE(state))
    {
        runtimeError(vm, "Error, object is not a table.");
        return 0;
    }
    else if (!IS_NUM(var))
    {
        runtimeError(vm, "Error, index is not a number.");
        return 0;
    }

    ObjTable* table = AS_TABLE(state);
    int idx = AS_NUM(var) + 1;
    Value* result = otGeti(idx, table);

    if (IS_NIL(result))
    {
        pushStack(NIL_CONSTANT, vm);
        pushStack(NIL_CONSTANT, vm);
        return 2;
    }
    else
    {
        pushStack(NUM_VAL(idx), vm);
        pushStackPtr(result, vm);
        return 2;
    }
}

uint8_t lib_ipairs(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    Value* table = &frame->slots[0];

    if (!IS_TABLE(table))
    {
        runtimeError(vm, "Error, object is not a table.");
        return 0;
    }

    ObjNativeFunction* iter = newNativeFunction(ipairsAux, vm);

    pushStack(NATIVE_VAL(iter), vm);
    pushStackPtr(table, vm);
    pushStack(NUM_VAL(0), vm);

    return 3;
}

static char* replacePattern(const char* filename, int nameLen, const char* pattern, int patternLen)
{
    char* path = malloc(patternLen + nameLen);
    char* curr = path;

    for (int i = 0; i < patternLen; i++)
    {
        char c = pattern[i];

        if (c == '?')
        {
            memcpy(curr, filename, nameLen);
            curr += nameLen;

            if (curr - path < patternLen + nameLen)
            {
                memcpy(curr, pattern + i + 1, patternLen - i - 1);
            }
            break;
        }
        else
        {
            *curr++ = c;
        }
    }

    path[patternLen + nameLen - 1] = '\0';
    return path;
}

static bool requireAux(const char* filename, int nameLen, VM* vm)
{
    const char* finalPath = NULL;

    for (uint8_t i = 0; i < DEFAULT_PATTERN_COUNT; i++)
    {
        Field* field = &DefaultPatterns[i];
        const char* moduleName = replacePattern(filename, nameLen, field->s, field->len);

        if (fileExists(moduleName))
        {
            finalPath = moduleName;
            break;
        }
    }

    if (finalPath == NULL)
    {
        return false;
    }

    /*
       spawn a subroutine here that shares crucial states with the previous vm but has a unique
       global table. this doesn't work because then i would have to copy the table over, and that
       would be very cubersome.
    */
    VM routine;
    initVM(G(vm), true, &routine);

    char* source = readSourceFile(finalPath);
    ObjFunction* f = compile(source, &routine);
    ObjClosure* newChunk = newClosure(f, &routine);

    unsafe_pop(&routine);
    unsafe_push(&routine, CLOSURE_VAL(newChunk));

    // this loads a new frame onto the current vm
    precall(0, SINGLERET, &routine);

    /*
       executes the new frame and removes it. when the chunk ends, it will end in an OP_RETURN that
       would either return null or return a single value (the module itself)
    */
    InterpretResult subRes = run(&routine);
    if (subRes == INTERPRET_ERROR)
    {
        return false;
    }

    unsafe_push(&routine, IS_NIL(stackat(&routine, 1)) ? FALSE_CONSTANT : TRUE_CONSTANT);

    return true;
}

uint8_t lib_require(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);
    Value* arg1 = &frame->slots[0];

    if (!IS_STRING(arg1))
    {
        fprintf(stderr, "Error, required path is not a string.");
        return 0;
    }

    ObjString* path = AS_STRING(arg1);
    if (!requireAux(path->chars, path->length, vm))
    {
        return 0;
    }

    return 2;
}

LibExport BASE_LIB[] = { { "print", lib_print }, { "ipairs", lib_ipairs },
    { "require", lib_require }, { NULL, NULL } };
