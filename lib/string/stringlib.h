#ifndef stringlib_h
#define stringlib_h

#include <libutils.h>
#include <vm.h>

uint8_t lib_len(uint8_t narg, VM* vm);

extern LibExport STRING_LIB[];

#endif
