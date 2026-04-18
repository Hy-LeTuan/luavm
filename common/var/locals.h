#ifndef common_var_locals_h
#define common_var_locals_h

#include <stddef.h>

typedef struct
{
    const char* start;
    int length;
    int scope;
} Local;

typedef struct
{
    size_t capacity;
    size_t count;
    Local* locals;
} LocalArray;

void initLocalArray(LocalArray* array);
void writeLocalArray(LocalArray* array, const char* start, int length, int scope);
void freeLocalArray(LocalArray* array);

#endif
