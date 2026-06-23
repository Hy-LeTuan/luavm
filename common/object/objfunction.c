#include <object.h>

#include <chunk.h>
#include <memory.h>

ObjFunction* newFunction(ObjFunction* parent, VM* vm)
{
    ObjFunction* f = ALLOCATE_OBJ(OBJ_FUNCTION, ObjFunction, vm);
    initChunk(&f->chunk);

    f->ip = f->chunk.code;
    f->arity = 0;
    f->upvalueCount = 0;
    f->type = TYPE_FUNCTION;

    f->esize = 0;
    f->enclosed = NULL;

    if (parent != NULL)
    {
        unsafe_push(vm, FUNCTION_VAL(f));
        // move to 1 enclosing
        parent->enclosed =
          REALLOCATE(parent->enclosed, parent->esize, parent->esize + 1, ObjFunction*, vm);
        parent->enclosed[parent->esize++] = f;
        unsafe_pop(vm);
    }

    return f;
}
