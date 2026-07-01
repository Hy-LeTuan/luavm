#include <gc.h>

#include <memory.h>
#include <chunk.h>
#include <metatable.h>
#include <value.h>
#include <vmstate.h>

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

#ifdef DEBUG_LOG_SWEEP
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

void markObject(Object* obj, ubyte markVal)
{
    if (obj == NULL || obj->marked == markVal)
    {
        return;
    }

#ifdef DEBUG_LOG_SWEEP
    printf("%p mark obj ", (void*)obj);
    Value objVal = OBJ_VAL(otype(obj), obj);
    printValue(&objVal);
    printf(" of type %d", vtype(obj));
    printf("\n");
#endif

    obj->marked = markVal;

    /*
       trace the references that the object also keeps track of
     */
    switch (otype(obj))
    {
        case OBJ_TABLE:
        {
            ObjTable* t = castobjto(ObjTable, obj);
            markObject(baseobj(t->mt), markVal);

            // mark array part
            for (size_t i = 0; i < t->array.count; i++)
            {
                markValue(&t->array.values[i], markVal);
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
                markValue(&f->chunk.constants[i], markVal);
            }

            // mark all enclosing functions
            for (int i = 0; i < f->esize; i++)
            {
                markObject(baseobj(f->enclosed[i]), markVal);
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
                    markObject(baseobj(closure->upvalues[i]), markVal);
                }
            }

            // mark base function
            markObject(baseobj(closure->function), markVal);
            break;
        }
        case OBJ_UPVALUE:
        {
            ObjUpvalue* uv = castobjto(ObjUpvalue, obj);
            markValue(uv->location, markVal);
            break;
        }
        default:
            return;
    }
}

void markSingleObject(Object* obj, ubyte markedVal)
{
    if (obj == NULL || obj->marked == markedVal)
    {
        return;
    }

    obj->marked = markedVal;
}

void markValue(Value* v, ubyte markVal)
{
    if (IS_OBJ(v))
    {
        markObject(AS_OBJ(v), markVal);
    }
}

void markTableWeak(ObjTable* t)
{
    /* mark only the table and nothing else */
    markSingleObject(baseobj(t), GC_MARKED);
}

void markTable(Table* t)
{
    if (t == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_SWEEP
    printf("%p mark table \n", (void*)t);
#endif

    for (size_t i = 0; i < t->capacity; i++)
    {
        Entry* entry = &t->entries[i];
        markValue(&entry->key, GC_MARKED);
        markValue(&entry->value, GC_MARKED);
    }
}

static void markRoots(VM* vm)
{
    /* mark values on stack */
    for (Value* slot = G(vm)->stack; slot < G(vm)->stackTop; slot++)
    {
        markValue(slot, GC_MARKED);
    }

    for (int i = 0; i < vm->cacheSize; i++)
    {
        markValue(&vm->cache[i], GC_MARKED);
    }

    /* mark closures on frame */
    for (size_t i = 0; i < G(vm)->frameCount; i++)
    {
        markObject(baseobj(G(vm)->frames[i].closure), GC_MARKED);
    }

    /* mark upvalues still reachable */
    for (ObjUpvalue* uv = vm->openUpvalues; uv != NULL; uv = uv->next)
    {
        markObject(baseobj(uv), GC_MARKED);
    }

    /* mark values on globals */
    markObject(baseobj(vm->globals), GC_MARKED);

    /* mark weak ref tables */
    markTableWeak(G(vm)->strings);

    /* mark metatables */
    for (uint8_t i = 0; i < MT_SIZE; i++)
    {
        markObject(baseobj(getmtdirect(vm, i)), GC_MARKED);
    }

    /* mark events */
    for (uint8_t i = 0; i < EVENT_SIZE; i++)
    {
        markObject(baseobj(getevent(vm, i)), GC_MARKED);
    }
}

static void sweep(VM* vm)
{
    Object* prev = NULL;
    Object* obj = G(vm)->objectStack;
    while (obj != NULL)
    {
        if (obj->marked)
        {
            if (obj->marked == GC_MARKED)
            {
                obj->marked = GC_RELEASE;
            }

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
                G(vm)->objectStack = obj;
            }

            freeObject(unreached, vm);
        }
    }
}

/*
   clean up on the hash part of an objtable only
*/
static void hSetCleanDangling(ObjTable* t, VM* vm)
{
    if (t == NULL)
    {
        return;
    }

    for (size_t i = 0; i < TABLE(t)->capacity; i++)
    {
        Entry* entry = &TABLE(t)->entries[i];

        if (!IS_NIL(&entry->key) && !AS_OBJ(&entry->key)->marked)
        {
            Value key = entry->key;
            tableErase(&key, TABLE(t));
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
    size_t before = vm->bytesAllocated;
#endif

    markRoots(vm);

    // clean up and sweep
    hSetCleanDangling(G(vm)->strings, vm);
    sweep(vm);
    G(vm)->GCthreshold = G(vm)->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("--- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm->bytesAllocated,
      before, vm->bytesAllocated, vm->GCthreshold);
#endif
}
