#ifndef obj_objtable_h
#define obj_objtable_h

#include <object.h>

void otSet(Value k, Value v, ObjTable* t);
void otSeti(int i, Value v, ObjTable* t);
void otSetRaw(Value k, Value v, ObjTable* t);

Value otGeti(int i, ObjTable* t);
Value otGet(Value k, ObjTable* t);
Value otGetRaw(Value k, ObjTable* t);

Value otRemove(Value k, ObjTable* t);

uint8_t otGetLen(ObjTable* t);

#endif
