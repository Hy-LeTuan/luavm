#ifndef lib_baselib_h
#define lib_baselib_h

#include <value.h>
#include <vm.h>

#include <stdint.h>

uint8_t lib_print(uint8_t narg, VM* vm);
uint8_t lib_ipairs(uint8_t narg, VM* vm);

#endif
