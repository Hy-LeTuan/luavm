#include <hash.h>

#include <object.h>

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

uint32_t hashString(const char* str, int l, HashFn f)
{
    return f(str, l);
}

static const void* valueToByte(const Value* value, int* byte_length)
{
#define TO_OBJ(type, v) ((type*)((v)->as.object))
    const void* bytes = NULL;

    switch (vtype(value))
    {
        case NUMBER:
            bytes = (void*)(&value->as.number);
            *byte_length = sizeof(value->as.number);
            break;
        case NIL:
            bytes = (void*)(&NIL_CONSTANT);
            *byte_length = sizeof(Value*);
            break;
        case BOOL:
            bytes = (void*)(&value->as.boolean);
            *byte_length = sizeof(value->as.boolean);
            break;
        case OBJ_STRING:
        {
            ObjString* string = TO_OBJ(ObjString, value);
            bytes = (void*)string->chars;
            *byte_length = string->length;
            break;
        }
        case OBJ_FUNCTION:
        {
            bytes = &value->as.object;
            *byte_length = (int)sizeof(Object*);
            break;
        }
        case OBJ_CLOSURE:
        {
            bytes = &value->as.object;
            *byte_length = (int)sizeof(Object*);
            break;
        }
        case OBJ_NATIVE:
        {
            bytes = &value->as.object;
            *byte_length = (int)sizeof(Object*);
            break;
        }
        case OBJ_TABLE:
        {
            bytes = &value->as.object;
            *byte_length = (int)sizeof(Object*);
            break;
        }
        case OBJ_UPVALUE:
        {
            bytes = &value->as.object;
            *byte_length = (int)sizeof(Object*);
            break;
        }
    }

    return bytes;
#undef TO_OBJ
}

uint32_t generateHash(const Value* value, HashFn hashFunc)
{
    int value_length;
    const void* value_bytes = valueToByte(value, &value_length);
    uint32_t hash = hashFunc(value_bytes, value_length);

    return hash;
}
