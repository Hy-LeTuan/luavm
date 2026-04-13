#ifndef common_table_hash_h
#define common_table_hash_h

#include <value.h>
#include <stdint.h>

extern const uint32_t FnvDefaultPrime;
extern const uint32_t FnvDefaultOffsetBasis;

uint32_t fnv1a_32(const void* bytes, int length);
void* valueToByte(Value value, int* byte_length);

#endif
