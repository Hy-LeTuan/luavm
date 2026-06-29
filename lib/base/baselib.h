#ifndef lib_baselib_h
#define lib_baselib_h

#include <libutils.h>
#include <value.h>
#include <vmstate.h>
#include <vmdo.h>

#include <stdint.h>

uint8_t lib_print(uint8_t narg, VM* vm);
uint8_t lib_ipairs(uint8_t narg, VM* vm);
uint8_t lib_require(uint8_t narg, VM* vm);

typedef struct
{
    const char* s;
    int len;
} Field;

extern LibExport BASE_LIB[];

#endif
