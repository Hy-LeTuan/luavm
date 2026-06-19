#ifndef stringlib_h
#define stringlib_h

#include <libutils.h>
#include <vmstate.h>
#include <vmdo.h>

uint8_t lib_len(uint8_t narg, VM* vm);
uint8_t lib_lower(uint8_t narg, VM* vm);
uint8_t lib_upper(uint8_t narg, VM* vm);
uint8_t lib_reverse(uint8_t narg, VM* vm);
uint8_t lib_rep(uint8_t narg, VM* vm);

extern LibExport STRING_LIB[];

#endif
