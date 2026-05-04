#include <native_functions.h>

#include <objstring.h>

#include <stdio.h>
#include <math.h>

Value print(Value* start)
{
    if (IS_NUM(*start))
    {
        float num = AS_NUM(*start);

        if (floor(num) == num)
        {
            fprintf(stdout, "%d\n", (int)num);
        }
        else
        {
            fprintf(stdout, "%.2f\n", AS_NUM(*start));
        }
    }
    else if (IS_BOOL(*start))
    {
        fprintf(stdout, "%s\n", AS_BOOL(*start) ? "true" : "false");
    }
    else if (IS_OBJ(*start))
    {
        printObject(AS_OBJ(*start));
        fprintf(stdout, "\n");
    }

    return NIL_CONSTANT;
}
