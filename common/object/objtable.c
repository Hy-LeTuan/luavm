#include <objtable.h>

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
    if (IS_NUM(k))
    {
        otSeti(AS_NUM(k), v, t);
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

    return tableGet(NUM_VAL(i + 1), TABLE(t));
}

Value otGet(Value k, ObjTable* t)
{
    if (IS_NUM(k))
    {
        return otGeti(AS_NUM(k), t);
    }

    return tableGet(k, TABLE(t));
}

Value otGetRaw(Value k, ObjTable* t)
{
    return tableGet(k, TABLE(t));
}

uint8_t otGetLen(ObjTable* t)
{
    // TODO: implement length for table
    return t->array.count;
}
