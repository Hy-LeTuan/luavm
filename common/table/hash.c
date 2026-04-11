#include <hash.h>

const uint32_t FnvDefaultPrime = 0x01000193U;
const uint32_t FnvDefaultOffsetBasis = 0x811C9DC5U;

uint32_t fnv1a_32(const void* bytes, int length)
{
    const char* bytes_ptr = (const char*)bytes;
    uint32_t hash = FnvDefaultOffsetBasis;

    for (int i = 0; i < length; i++)
    {
        hash *= FnvDefaultPrime;
        hash = hash ^ (uint32_t)(bytes_ptr[i]);
    }

    return hash;
}
