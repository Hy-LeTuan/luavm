#ifndef common_object_gc_h
#define common_object_gc_h

#include <object.h>

#define CAN_GC(v)                                                                                  \
    (vtype(v) == OBJ_STRING || vtype(v) == OBJ_TABLE || vtype(v) == OBJ_FUNCTION ||                \
      vtype(v) == OBJ_CLOSURE || vtype(v) == OBJ_NATIVE || vtype(v) == OBJ_UPVALUE)

void freeObject(Object* obj);
void freeObjects(Object* objects);

#endif
