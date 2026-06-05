#ifndef lib_baselib_h
#define lib_baselib_h

#include <value.h>
#include <vm.h>

#include <stdint.h>

#define castvm(ptr) ((VM*)ptr)

uint8_t lib_print(uint8_t narg, void* info);
uint8_t lib_ipairs(uint8_t narg, void* info);

#endif
