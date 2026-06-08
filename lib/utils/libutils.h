#ifndef lib_libutils_h
#define lib_libutils_h

#include <object.h>

typedef struct
{
    const char* name;
    NativeFn f;
} LibExport;

#endif
