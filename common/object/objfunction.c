#include "object.h"
#include <objfunction.h>

ObjFunction* newFunction(int arity)
{
    ObjFunction* f = ALLOCATE_OBJ(OBJ_FUNCTION, ObjFunction);

    initChunk(&f->chunk);

    f->ip = f->chunk.code;
    f->arity = arity;

    return f;
}
