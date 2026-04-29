#include <objfunction.h>

#include <object.h>

ObjFunction* newFunction()
{
    ObjFunction* f = ALLOCATE_OBJ(OBJ_FUNCTION, ObjFunction);

    initChunk(&f->chunk);

    f->ip = f->chunk.code;
    f->arity = 0;

    return f;
}
