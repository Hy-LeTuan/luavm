#ifndef common_object_gc_h
#define common_object_gc_h

#include <object.h>
#include <vmstate.h>

#define CAN_GC(v)                                                                                  \
    (vtype(v) == OBJ_STRING || vtype(v) == OBJ_TABLE || vtype(v) == OBJ_FUNCTION ||                \
      vtype(v) == OBJ_CLOSURE || vtype(v) == OBJ_NATIVE || vtype(v) == OBJ_UPVALUE)

void freeObject(Object* obj, VM* vm);
void freeObjects(Object* objects, VM* vm);
void markValue(Value* v);
void markObject(Object* obj);
void markTable(Table* t);
void collectGarbage(VM* vm);

#endif
