#ifndef common_var_locals_h
#define common_var_locals_h

#include <stddef.h>

typedef struct
{
    const char* start;
    int length;
    int scope;
    bool isCaptured;
} Local;

typedef struct
{
    size_t capacity;
    size_t count;
    Local* locals;
} LocalArray;

#endif
