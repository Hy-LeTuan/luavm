#include <lapi.h>

#include <compiler.h>
#include <baselib.h>
#include <stringlib.h>
#include <tablelib.h>
#include <objtable.h>
#include <mathlib.h>
#include <gc.h>

#include <string.h>
// #include <stdio.h>

#define setvmmetatable(type, value) (G(vm)->mts[type] = value)

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
        lock_object(baseobj(key));

        ObjNativeFunction* native = newNativeFunction(func, vm);
        lock_object(baseobj(native));

        tableInsertOrSet(STRING_VAL(key), NATIVE_VAL(native), lib, vm);

        release_object(baseobj(key));
        release_object(baseobj(native));
    }
}

static void insertToLibGlobal(const char* name, Value v, VM* vm)
{
    ObjString* objname = copyString(name, strlen(name), vm);
    unsafe_push(vm, STRING_VAL(objname));

    tableInsertOrSet(STRING_VAL(objname), v, TABLE(G(vm)->libGlobals), vm);

    unsafe_pop(vm);
}

static ObjTable* createStandardMetatable(VM* vm)
{
    ObjTable* table = newObjTable(vm);
    unsafe_push(vm, TABLE_VAL(table));

    for (uint8_t i = 0; i < EVENT_SIZE; i++)
    {
        ObjString* event = getevent(vm, i);
        otSetRaw(STRING_VAL(event), NIL_CONSTANT, table, vm);
    }

    unsafe_pop(vm);
    return table;
}

static void defineMtsAndEnvs(VM* vm)
{
    /*
       define all libraries
    */
    defineLib(BASE_LIB, TABLE(G(vm)->libGlobals), vm);

    ObjTable* stringlib = newObjTable(vm);
    unsafe_push(vm, TABLE_VAL(stringlib));
    defineLib(STRING_LIB, TABLE(stringlib), vm);
    insertToLibGlobal("string", TABLE_VAL(stringlib), vm);
    unsafe_pop(vm);

    ObjTable* tablelib = newObjTable(vm);
    unsafe_push(vm, TABLE_VAL(tablelib));
    defineLib(TABLE_LIB, TABLE(tablelib), vm);
    insertToLibGlobal("table", TABLE_VAL(tablelib), vm);
    unsafe_pop(vm);

    ObjTable* mathlib = newObjTable(vm);
    unsafe_push(vm, TABLE_VAL(mathlib));
    defineLib(MATH_LIB, TABLE(mathlib), vm);
    insertToLibGlobal("math", TABLE_VAL(mathlib), vm);
    unsafe_pop(vm);

    /*
       define metatables
    */
    ObjTable* stringmt = createStandardMetatable(vm);
    lock_object(baseobj(stringmt));
    otSetRaw(STRING_VAL(getevent(vm, EVENT_INDEX)), TABLE_VAL(stringlib), stringmt, vm);
    release_object(baseobj(stringmt));
    setvmmetatable(OBJ_STRING, stringmt);
}

void setupSingleChunkVM(const char* source, GlobalState* g, VM* vm)
{
    initVM(g, false, vm);
    defineMtsAndEnvs(vm);

    ObjFunction* function = compile(source, vm);
    ObjClosure* closure = newClosure(function, vm);

    // remove the old function and push closure
    unsafe_pop(vm);
    unsafe_push(vm, CLOSURE_VAL(closure));

    // call main script
    precall(0, ZERORET, vm);
}

InterpretResult execChunkST(const char* source)
{
    VM vm;
    GlobalState g;

    setupSingleChunkVM(source, &g, &vm);
    InterpretResult result = run(&vm);
    freeVM(&vm);

    return result;
}
