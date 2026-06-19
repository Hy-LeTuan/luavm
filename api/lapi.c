#include <lapi.h>

#include <compiler.h>
#include <baselib.h>
#include <stringlib.h>
#include <tablelib.h>
#include <objtable.h>
#include <mathlib.h>

#include <string.h>

#define setvmmetatable(type, value) (vm->mts[type] = value)

/*
   @param lib: The library that matches the function name with the actual function
*/
static void defineLib(LibExport* libExport, Table* lib, VM* vm)
{
    for (;; libExport++)
    {
        const char* name = libExport->name;
        NativeFn func = libExport->f;

        if (name == NULL || func == NULL)
        {
            return;
        }

        ObjString* key = copyString(name, strlen(name), vm);
        ObjNativeFunction* native = newNativeFunction(func, vm);
        linkObject(baseobj(key), vm);
        linkObject(baseobj(native), vm);
        tableInsertOrSet(STRING_VAL(key), NATIVE_VAL(native), lib, vm);
    }
}

static void insertToGlobal(const char* name, Value v, VM* vm)
{
    ObjString* objname = copyString(name, strlen(name), vm);
    linkObject(baseobj(objname), vm);
    tableInsertOrSet(STRING_VAL(objname), v, &vm->globals, vm);
}

static ObjTable* createStandardMetatable(VM* vm)
{
    ObjTable* table = newObjTable(vm);

    for (uint8_t i = 0; i < EVENT_SIZE; i++)
    {
        ObjString* event = vm->events[i];
        otSetRaw(STRING_VAL(event), NIL_CONSTANT, table, vm);
    }

    return table;
}

static void defineMtsAndEnvs(VM* vm)
{
    /*
       define all libraries
    */
    defineLib(BASE_LIB, &vm->globals, vm);

    ObjTable* stringlib = newObjTable(vm);
    defineLib(STRING_LIB, TABLE(stringlib), vm);
    insertToGlobal("string", TABLE_VAL(stringlib), vm);

    ObjTable* tablelib = newObjTable(vm);
    defineLib(TABLE_LIB, TABLE(tablelib), vm);
    insertToGlobal("table", TABLE_VAL(tablelib), vm);

    ObjTable* mathlib = newObjTable(vm);
    defineLib(MATH_LIB, TABLE(mathlib), vm);
    insertToGlobal("math", TABLE_VAL(mathlib), vm);

    /*
       define metatables
    */
    ObjTable* stringmt = createStandardMetatable(vm);
    otSetRaw(STRING_VAL(vm->events[EVENT_INDEX]), TABLE_VAL(stringlib), stringmt, vm);

    setvmmetatable(OBJ_STRING, stringmt);
}

void setupSingleChunkVM(const char* source, VM* vm)
{
    initVM(vm);
    defineMtsAndEnvs(vm);

    ObjFunction* function = compile(source, vm);
    ObjClosure* closure = newClosure(function, vm);

    // remove the old function
    popStack(vm);

    // push the closure and call main script
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
