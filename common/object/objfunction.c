#include <object.h>

#include <chunk.h>
#include <memory.h>

static ObjFunction* subscribeTo(ObjFunction* parent, VM* vm)
{
    ObjFunction* f = ALLOCATE_OBJ(OBJ_FUNCTION, ObjFunction, vm);

    if (parent != NULL)
    {
        // move to 1 enclosing
        parent->enclosed =
          REALLOCATE(parent->enclosed, parent->esize, parent->esize + 1, ObjFunction*, vm);
        parent->enclosed[parent->esize++] = f;
    }

    return f;
}

ObjFunction* newFunction(ObjFunction* parent, VM* vm)
{
    ObjFunction* f = subscribeTo(parent, vm);

    initChunk(&f->chunk);

    f->ip = f->chunk.code;
    f->arity = 0;
    f->upvalueCount = 0;
    f->type = TYPE_FUNCTION;

    f->esize = 0;
    f->enclosed = NULL;

    return f;
}
