#include <object.h>

#include <memory.h>
#include <vmstate.h>

#include <stdlib.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif

Object* allocateObj(ValueType type, size_t size, VM* vm)
{
    Object* obj = ALLOCATE(Object, size, vm);
    obj->type = type;
    obj->marked = 0;
    obj->next = NULL;

    (obj)->next = G(vm)->objectStack;
    G(vm)->objectStack = obj;

#ifdef DEBUG_LOG_SWEEP
    printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif

    return obj;
}
