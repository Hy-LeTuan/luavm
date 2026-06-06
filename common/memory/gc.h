#ifndef common_object_gc_h
#define common_object_gc_h

#include <object.h>

void freeObject(Object* obj);
void freeObjects(Object* objects);

#endif
