#ifndef common_object_gc_h
#define common_object_gc_h

#include <object.h>
#include <vmstate.h>

#define CAN_GC(v)                                                                                  \
    (vtype(v) == OBJ_STRING || vtype(v) == OBJ_TABLE || vtype(v) == OBJ_FUNCTION ||                \
      vtype(v) == OBJ_CLOSURE || vtype(v) == OBJ_NATIVE || vtype(v) == OBJ_UPVALUE)

#define GC_LOCKED 2
#define GC_MARKED 1
#define GC_RELEASE 0

#define lock_object(obj) (markObject(obj, GC_LOCKED))
#define release_object(obj) (markObject(obj, GC_RELEASE))

#define lock_value(v) (markValue(v, GC_LOCKED))
#define release_value(v) (markValue(v, GC_RELEASE))

void freeObject(Object* obj, VM* vm);
void freeObjects(Object* objects, VM* vm);
void markValue(Value* v, ubyte makrVal);
void markObject(Object* obj, ubyte markVal);
void markSingleObject(Object* obj, ubyte markedVal);
void markTableWeak(ObjTable* t);
void markTable(Table* t);
void collectGarbage(VM* vm);

#endif
