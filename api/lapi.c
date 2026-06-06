#include <lapi.h>

#include <object.h>
#include <compiler.h>
#include <baselib.h>

static void defineNativeFunc(const char* name, int length, NativeFn function, VM* vm)
{
    ObjString* key = copyString(name, length, &vm->strings);
    ObjNativeFunction* native = newNativeFunction(function);

    linkObject(baseobj(key), vm);
    linkObject(baseobj(native), vm);

    tableInsertOrSet(STRING_VAL(key), NATIVE_VAL(native), &vm->globals);
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

    pushStack(CLOSURE_VAL(closure), vm);
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
