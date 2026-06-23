#include <gc.h>

#include <memory.h>
#include <chunk.h>
#include <metatable.h>
#include <value.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include <stdlib.h>
#endif

void freeObject(Object* obj, VM* vm)
{
    if (obj == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p free obj ", (void*)obj);

    Value original = OBJ_VAL(otype(obj), obj);
    printValue(&original);

    printf(" of type: %d\n", otype(obj));
#endif

    switch (otype(obj))
    {
        case OBJ_STRING:
        {
            ObjString* string = castobjto(ObjString, obj);
            FREE_ARRAY(string->chars, string->length + 1, char, vm);
            FREE(string, ObjString, vm);
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*)obj;
            freeChunk(&function->chunk, vm);
            FREE_ARRAY(function->enclosed, function->esize, ObjFunction*, vm);
            FREE(function, ObjFunction, vm);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)obj;
            FREE_ARRAY(closure->upvalues, closure->function->upvalueCount, ObjUpvalue*, vm);
            FREE(closure, ObjClosure, vm);
            break;
        }
        case OBJ_UPVALUE:
        {
            FREE(obj, ObjUpvalue, vm);
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(obj, ObjNativeFunction, vm);
            break;
        }
        case OBJ_TABLE:
        {
            ObjTable* table = (ObjTable*)obj;
            freeTable(&table->content, vm);
            freeValueArray(&table->array, vm);
            FREE(table, ObjTable, vm);
            break;
        }
        default:
            return;
    }
}

void freeObjects(Object* objects, VM* vm)
{
    Object* p = objects;

    while (p != NULL)
    {
        Object* next = p->next;
        freeObject(p, vm);
        p = next;
    }
}

void markObject(Object* obj)
{
    if (obj == NULL || obj->marked)
    {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark obj ", (void*)obj);
    Value objVal = OBJ_VAL(otype(obj), obj);
    printValue(&objVal);
    printf(" of type %d", vtype(obj));
    printf("\n");
#endif

    obj->marked = true;

    /*
       trace the references that the object also keeps track of
     */
    switch (otype(obj))
    {
        case OBJ_TABLE:
        {
            ObjTable* t = castobjto(ObjTable, obj);
            markObject(baseobj(t->mt));

            // mark array part
            for (size_t i = 0; i < t->array.count; i++)
            {
                markValue(&t->array.values[i]);
            }

            // mark table part
            markTable(TABLE(t));
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction* f = castobjto(ObjFunction, obj);

            for (size_t i = 0; i < f->chunk.ccount; i++)
            {
                markValue(&f->chunk.constants[i]);
            }

            // mark all enclosing functions
            for (int i = 0; i < f->esize; i++)
            {
                markObject(baseobj(f->enclosed[i]));
            }

            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = castobjto(ObjClosure, obj);

            // mark its upvalues
            // printf("closure upvalue count is: %d\n", closure->function->upvalueCount);

            if (closure->upvalues != NULL && closure->function->upvalueCount > 0)
            {
                for (int i = 0; i < closure->function->upvalueCount; i++)
                {
                    markObject(baseobj(closure->upvalues[i]));
                }
            }

            // mark base function
            markObject(baseobj(closure->function));
            break;
        }
        case OBJ_UPVALUE:
        {
            ObjUpvalue* uv = castobjto(ObjUpvalue, obj);
            markValue(uv->location);
            break;
        }
        default:
            return;
    }
}

void markValue(Value* v)
{
    if (IS_OBJ(v))
    {
        markObject(AS_OBJ(v));
    }
}

void markTable(Table* t)
{
    if (t == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark table \n", (void*)t);
#endif

    for (size_t i = 0; i < t->capacity; i++)
    {
        Entry* entry = &t->entries[i];
        markValue(&entry->key);
        markValue(&entry->value);
    }
}

static void markRoots(VM* vm)
{
    /* mark values on stack */
    for (Value* slot = vm->stack; slot < vm->stackTop; slot++)
    {
        markValue(slot);
    }

#ifdef DEBUG_LOG_GC
    printf("cache size is: %d\n", vm->cacheSize);
#endif

    for (int i = 0; i < vm->cacheSize; i++)
    {
        markValue(&vm->cache[i]);
    }

    /* mark closures on frame */
    for (size_t i = 0; i < vm->frameCount; i++)
    {
        markObject(baseobj(vm->frames[i].closure));
    }

    /* mark upvalues still reachable */
    for (ObjUpvalue* uv = vm->openUpvalues; uv != NULL; uv = uv->next)
    {
        markObject(baseobj(uv));
    }

    /* mark values on globals */
    markTable(&vm->globals);

    /* mark metatables */
    for (uint8_t i = 0; i < MT_SIZE; i++)
    {
        markObject(baseobj(vm->mts[i]));
    }

    /* mark events */
    for (uint8_t i = 0; i < EVENT_SIZE; i++)
    {
        markObject(baseobj(vm->events[i]));
    }
}

static void sweep(VM* vm)
{
    Object* prev = NULL;
    Object* obj = vm->objectStack;
    while (obj != NULL)
    {
        if (obj->marked)
        {
            obj->marked = false;
            prev = obj;
            obj = obj->next;
        }
        else
        {
            Object* unreached = obj;
            obj = obj->next;
            if (prev != NULL)
            {
                prev->next = obj;
            }
            else
            {
                vm->objectStack = obj;
            }

            freeObject(unreached, vm);
        }
    }
}

static void hSetCleanDangling(Table* t, VM* vm)
{
    for (size_t i = 0; i < t->capacity; i++)
    {
        Entry* entry = &t->entries[i];

        if (!IS_NIL(&entry->key) && !AS_OBJ(&entry->key)->marked)
        {
            Value key = entry->key;
            tableErase(&key, t);
        }
    }
}

void collectGarbage(VM* vm)
{
#ifdef DEBUG_LOG_GC
    /* vm should never be null */
    if (vm == NULL)
    {
        fprintf(stderr, "Error, vm is null when passed to gc.\n");
        exit(EXIT_FAILURE);
    }

    printf("--- gc begin\n");
#endif

    markRoots(vm);

    // clean up and sweep
    hSetCleanDangling(&vm->strings, vm);
    sweep(vm);

#ifdef DEBUG_LOG_GC
    printf("--- gc end\n");
#endif
}
