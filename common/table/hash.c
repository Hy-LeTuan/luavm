#include <hash.h>

#include <object.h>
#include <objstring.h>

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

void* valueToByte(Value value, int* byte_length)
{
    void* bytes = NULL;

    switch (value.type)
    {
        case NUMBER:
            bytes = (void*)(&value.as.number);
            *byte_length = sizeof(value.as.number);
            break;
        case BOOL:
            bytes = (void*)(&value.as.boolean);
            *byte_length = sizeof(value.as.boolean);
            break;
        case OBJECT:
        {
            Object* obj = value.as.object;
            switch (obj->type)
            {
                case OBJ_STRING:
                {
                    ObjString* string = AS_STRING(value);
                    bytes = (void*)string->chars;
                    *byte_length = string->length;
                    break;
                }
                case OBJ_TABLE:
                case OBJ_FUNCTION:
                    *byte_length = 0;
                    break;
            }
            break;
        }
        case NIL:
            *byte_length = 0;
            break;
    }

    return bytes;
}
