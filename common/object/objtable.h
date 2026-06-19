#ifndef obj_objtable_h
#define obj_objtable_h

#include <object.h>
#include <vmstate.h>

void otSet(Value k, Value v, ObjTable* t, VM* vm);
void otSeti(int i, Value v, ObjTable* t, VM* vm);
void otSetRaw(Value k, Value v, ObjTable* t, VM* vm);

Value* otGeti(int i, ObjTable* t);
Value* otGet(const Value* k, ObjTable* t);
Value* otGetRaw(const Value* k, ObjTable* t);
Value* otGetWithPtr(const char* c, int l, ObjTable* t);

Value otRemove(Value k, ObjTable* t);

int otGetLen(ObjTable* t);

#endif
