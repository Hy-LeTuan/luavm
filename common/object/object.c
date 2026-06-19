#include <object.h>

#include <memory.h>
#include <vmstate.h>

#include <stdlib.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif

#define linkobject(vm, obj)                                                                        \
    if ((obj) != NULL && (obj)->next == NULL)                                                      \
    {                                                                                              \
        (obj)->next = vm->objectStack;                                                             \
        vm->objectStack = obj;                                                                     \
    }

Object* allocateObj(ValueType type, size_t size, VM* vm)
{
    Object* obj = ALLOCATE(Object, size, vm);
    obj->type = type;
    obj->next = NULL;

    linkobject(vm, obj);

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif

    return obj;
}
