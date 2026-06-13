#include <objtable.h>

#include <limits.h>

ObjTable* newObjTable()
{
    ObjTable* table = ALLOCATE_OBJ(OBJ_TABLE, ObjTable);

    initTable(&table->content);
    initValueArray(&table->array);
    table->mt = NULL;

    return table;
}

void otSeti(int i, Value v, ObjTable* t)
{
    i--;

    if (i >= 0 && i < t->array.count)
    {
        t->array.values[i] = v;
    }
    else if (i == t->array.count)
    {
        writeValueArray(&t->array, v);
    }
    else
    {
        tableInsertOrSet(NUM_VAL(i + 1), v, TABLE(t));
    }
}

void otSet(Value k, Value v, ObjTable* t)
{
    if (IS_NUM(&k))
    {
        otSeti(AS_NUM(&k), v, t);
    }
    else
    {
        // hash that shit
        tableInsertOrSet(k, v, TABLE(t));
    }
}

void otSetRaw(Value k, Value v, ObjTable* t)
{
    tableInsertOrSet(k, v, TABLE(t));
}

Value otGeti(int i, ObjTable* t)
{
    i--;

    if (i >= 0 && i < t->array.count)
    {
        return t->array.values[i];
    }

    Value key = NUM_VAL(i + 1);
    return tableGet(&key, TABLE(t));
}

Value otGet(Value* k, ObjTable* t)
{
    if (IS_NUM(k))
    {
        return otGeti(AS_NUM(k), t);
    }

    return tableGet(k, TABLE(t));
}

Value otGetRaw(Value* k, ObjTable* t)
{
    return tableGet(k, TABLE(t));
}

Value otGetWithPtr(const char* c, int l, ObjTable* t)
{
    return tableGetWithPtr(c, l, TABLE(t));
}

static int unboundSearch(unsigned int idx, ObjTable* t)
{
    unsigned int i = idx;
    unsigned int j = idx + 1;

    while (otGeti(j, t).type != NIL)
    {
        i = j;
        j *= 2;

        if (j > INT_MAX)
        {
            i = 1;
            while (otGeti(i, t).type != NIL)
            {
                i++;
            }
            return i;
        }
    }

    while (i + 1 < j)
    {
        unsigned int m = (i + j) / 2;
        if (otGeti(m, t).type != NIL)
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
    // TODO: implement length for table
    // default: have a table to set size
    // if failed: look for a field `n` in the current array
    Value v = otGetWithPtr("n", 1, t);

    if (IS_NUM(&v))
    {
        return AS_NUM(&v);
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
