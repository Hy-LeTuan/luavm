#include <object.h>

#include <memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Object* allocateObj(ValueType type, size_t size)
{
    Object* obj = ALLOCATE(Object, size);
    obj->type = type;
    obj->next = NULL;

    return obj;
}
