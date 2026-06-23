#include <objtable.h>

#include <memory.h>
#include <limits.h>

void initValueArray(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value, VM* vm)
{
    if (array->count + 1 >= array->capacity)
    {
        int newCapacity = GROW_SIZE(array->capacity);
        array->values = REALLOCATE(array->values, array->capacity, newCapacity, Value, vm);
        array->capacity = newCapacity;
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array, VM* vm)
{
    FREE_ARRAY(array->values, array->capacity, Value, vm);
    array->capacity = 0;
    array->count = 0;
}

ObjTable* newObjTable(VM* vm)
{
    ObjTable* table = ALLOCATE_OBJ(OBJ_TABLE, ObjTable, vm);

    table->mt = NULL;
    initTable(&table->content);
    initValueArray(&table->array);

    return table;
}

void otSeti(int i, Value v, ObjTable* t, VM* vm)
{
    i--;

    if (i >= 0 && i < t->array.count)
    {
        t->array.values[i] = v;
    }
    else if (i == t->array.count)
    {
        writeValueArray(&t->array, v, vm);
    }
    else
    {
        tableInsertOrSet(NUM_VAL(i + 1), v, TABLE(t), vm);
    }
}

void otSet(Value k, Value v, ObjTable* t, VM* vm)
{
    if (IS_NUM(&k))
    {
        otSeti(AS_NUM(&k), v, t, vm);
    }
    else
    {
        tableInsertOrSet(k, v, TABLE(t), vm);
    }
}

void otSetRaw(Value k, Value v, ObjTable* t, VM* vm)
{
    tableInsertOrSet(k, v, TABLE(t), vm);
}

Value* otGeti(int i, ObjTable* t)
{
    i--;

    if (i >= 0 && i < t->array.count)
    {
        return &t->array.values[i];
    }

    Value key = NUM_VAL(i + 1);
    return tableGet(&key, TABLE(t));
}

Value* otGet(const Value* k, ObjTable* t)
{
    if (IS_NUM(k))
    {
        return otGeti(AS_NUM(k), t);
    }

    return tableGet(k, TABLE(t));
}

Value* otGetRaw(const Value* k, ObjTable* t)
{
    return tableGet(k, TABLE(t));
}

Value* otGetWithPtr(const char* c, int l, ObjTable* t)
{
    return tableGetWithPtr(c, l, TABLE(t));
}

static int unboundSearch(unsigned int idx, ObjTable* t)
{
    unsigned int i = idx;
    unsigned int j = idx + 1;

    while (!IS_NIL(otGeti(j, t)))
    {
        i = j;
        j *= 2;

        if (j > INT_MAX)
        {
            i = 1;
            while (!IS_NIL(otGeti(i, t)))
            {
                i++;
            }
            return i;
        }
    }

    while (i + 1 < j)
    {
        unsigned int m = (i + j) / 2;
        if (IS_NIL(otGeti(m, t)))
        {
            j = m;
        }
        else
        {
            i = m;
        }
    }
    return i;
}

int otGetLen(ObjTable* t)
{
    Value* v = otGetWithPtr("n", 1, t);

    if (IS_NUM(v))
    {
        return AS_NUM(v);
    }
    // else: find the first element that is nil next to an element that is not nil
    else
    {
        unsigned int j = t->array.count;
        if (j > 0 && IS_NIL(&t->array.values[j - 1]))
        {
            unsigned int i = 0;
            while (i + 1 < j)
            {
                unsigned int m = (i + j) / 2;
                if (IS_NIL(&t->array.values[m - 1]))
                {
                    j = m;
                }
                else
                {
                    i = m;
                }
            }
            return i;
        }
        /* hash part is empty? */
        else if (t->content.count == 0)
        {
            return j;
        }
        else
        {
            return unboundSearch(j, t);
        }
    }
}
