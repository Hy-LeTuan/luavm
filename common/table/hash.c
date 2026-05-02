#include <hash.h>

#include <object.h>
#include <objstring.h>
#include <objfunction.h>
#include <objclosure.h>
#include <objnativefunction.h>

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

void* valueToByte(const Value value, int* byte_length)
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
                case OBJ_FUNCTION:
                {
                    ObjFunction* function = (ObjFunction*)obj;
                    bytes = (void*)function;
                    *byte_length = (int)sizeof(ObjFunction*);
                    break;
                }
                case OBJ_CLOSURE:
                {
                    ObjClosure* closure = (ObjClosure*)obj;
                    bytes = (void*)closure;
                    *byte_length = (int)sizeof(ObjClosure*);
                    break;
                }
                case OBJ_NATIVE:
                {
                    ObjNativeFunction* native = (ObjNativeFunction*)obj;
                    bytes = (void*)native;
                    *byte_length = (int)sizeof(ObjNativeFunction*);
                    break;
                }
                case OBJ_TABLE:
                case OBJ_UPVALUE:
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

uint32_t generateHash(const Value value, HashFn hashFunc)
{
    int value_length;
    void* value_bytes = valueToByte(value, &value_length);
    uint32_t hash = hashFunc(value_bytes, value_length);

    return hash;
}
