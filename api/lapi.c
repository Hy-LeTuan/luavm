#include <lapi.h>

#include <compiler.h>
#include <objclosure.h>
#include <objnativefunction.h>
#include <objstring.h>
#include <baselib.h>

static void defineNativeFunc(const char* name, int length, NativeFn function, VM* vm)
{
    ObjString* key = copyString(name, length, &vm->strings);
    ObjNativeFunction* native = newNativeFunction(function);

    linkObject((Object*)key, vm);
    linkObject((Object*)native, vm);

    tableInsertOrSet(OBJ_VAL((Object*)key), OBJ_VAL((Object*)native), &vm->globals);
}

static void defineBaseLib(VM* vm)
{
    defineNativeFunc("print", 5, lib_print, vm);
    defineNativeFunc("ipairs", 6, lib_ipairs, vm);
}

void setupSingleChunkVM(const char* source, VM* vm)
{
    initVM(vm);
    defineBaseLib(vm);

    ObjFunction* function = compile(source, &vm->strings);
    ObjClosure* closure = newClosure(function);

    pushStack(OBJ_VAL((Object*)closure), vm);
    precall(0, 1, vm);
}

InterpretResult execChunkST(const char* source)
{
    VM vm;
    setupSingleChunkVM(source, &vm);
    InterpretResult result = run(&vm);
    freeVM(&vm);
    return result;
}
