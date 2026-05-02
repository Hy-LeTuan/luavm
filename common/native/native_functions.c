#include <native_functions.h>

#include <objstring.h>

#include <stdio.h>

Value print(Value* start)
{
    if (IS_NUM(*start))
    {
        fprintf(stdout, "%.2f\n", AS_NUM(*start));
    }
    else if (IS_BOOL(*start))
    {
        fprintf(stdout, "%s\n", AS_BOOL(*start) ? "true" : "false");
    }
    else if (IS_OBJ(*start))
    {
        printObject(AS_OBJ(*start));
    }

    return NIL_CONSTANT;
}
