#ifndef lib_mathlib_h
#define lib_mathlib_h

#include <libutils.h>
#include <vmstate.h>
#include <vmdo.h>

uint8_t lib_floor(uint8_t nargs, VM* vm);
uint8_t lib_random(uint8_t nargs, VM* vm);

extern LibExport MATH_LIB[];

#endif
