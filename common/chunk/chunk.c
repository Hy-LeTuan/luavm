#include <chunk.h>
#include <memory.h>
#include <gc.h>

#include <stdio.h>

void initChunk(Chunk* c)
{
    c->capacity = 0;
    c->count = 0;
    c->code = NULL;
    c->lines = NULL;

    c->ccount = 0;
    c->csize = 0;
    c->constants = NULL;
}

void writeChunk(Chunk* c, uint8_t op, size_t l, VM* vm)
{
    if (c->count + 1 >= c->capacity)
    {
        int newCapacity = GROW_SIZE(c->capacity);

        // grow op
        c->code = REALLOCATE(c->code, c->capacity, newCapacity, uint8_t, vm);

        // grow line
        c->lines = REALLOCATE(c->lines, c->capacity, newCapacity, size_t, vm);

        c->capacity = newCapacity;
    }

    c->code[c->count] = op;
    c->lines[c->count] = l;
    c->count++;
}

size_t getOpCodeLine(Chunk* c, int offset)
{
    return c->lines[offset];
}

static size_t writeConstant(Chunk* c, Value v, VM* vm)
{
    if (c->ccount + 1 >= c->csize)
    {
        size_t newSize = GROW_SIZE(c->csize);
        c->constants = REALLOCATE(c->constants, c->csize, newSize, Value, vm);
        c->csize = newSize;
    }

    c->constants[c->ccount++] = v;
    return c->ccount - 1;
}

size_t addConstant(Chunk* c, Value v, Table* lookup, VM* vm)
{
    Value* index = tableGet(&v, lookup);

    if (IS_NIL(index))
    {
        lock_value(&v);

        size_t vIdx = writeConstant(c, v, vm);
        tableInsertOrSet(v, NUM_VAL((LuaNum)vIdx), lookup, vm);

        release_value(&v);

        return vIdx;
    }

    return (size_t)AS_NUM(index);
}

void freeChunk(Chunk* c, VM* vm)
{
    FREE_ARRAY(c->code, c->capacity, uint8_t, vm);
    FREE_ARRAY(c->lines, c->capacity, size_t, vm);
    FREE_ARRAY(c->constants, c->csize, Value, vm);

    c->capacity = 0;
    c->count = 0;

    c->csize = 0;
    c->ccount = 0;
}
