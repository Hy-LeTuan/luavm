#ifndef common_table_hash_h
#define common_table_hash_h

#include <value.h>
#include <stdint.h>

typedef uint32_t (*HashFn)(const void*, int);

extern const uint32_t FnvDefaultPrime;
extern const uint32_t FnvDefaultOffsetBasis;

uint32_t fnv1a_32(const void* bytes, int length);
void* valueToByte(const Value value, int* byte_length);
uint32_t generateHash(const Value value, HashFn hashFunc);

#endif
