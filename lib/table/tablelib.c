#include <tablelib.h>

#include <objtable.h>

#define swap(x, y)                                                                                 \
    {                                                                                              \
        Value temp = x;                                                                            \
        x = y;                                                                                     \
        y = temp;                                                                                  \
    }

static void sortAux(int l, int r, ObjTable* t, Value compare)
{
    // TODO: use compare function or default compare operator '<' appropriately and not just assume
    // everything is an int

    if (l >= r)
    {
        return;
    }

    while (l < r)
    {
        LuaNum pivot = AS_NUM(t->array.values[r - 1]);

        int i = l;

        for (int j = l; j < r - 1; j++)
        {
            if (AS_NUM(t->array.values[j]) < pivot)
            {
                swap(t->array.values[i], t->array.values[j]);
                i++;
            }
        }

        swap(t->array.values[i], t->array.values[r - 1]);

        if (r - i > i - l)
        {
            sortAux(l, i, t, compare);
            l = i + 1;
        }
        else
        {
            sortAux(i + 1, r, t, compare);
            r = i;
        }
    }
}

uint8_t sort(uint8_t narg, VM* vm)
{
    CallFrame* frame = currframe(vm);

    Value arg1 = frame->slots[0];
    Value arg2 = frame->slots[1];

    if (!IS_TABLE(arg1))
    {
        return 0;
    }

    ObjTable* t = AS_TABLE(arg1);
    if (t->array.count == 0)
    {
        return 0;
    }

    sortAux(0, otGetLen(t), t, arg2);

    return 0;
}

LibExport TABLE_LIB[] = { { "sort", sort }, { NULL, NULL } };
