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

void* valueToByte(const Value* value, int* byte_length)
{
#define TO_OBJ(type, v) ((type*)((v)->as.object))
    void* bytes = NULL;

    switch (vtype(value))
    {
        case NUMBER:
            bytes = (void*)(&value->as.number);
            *byte_length = sizeof(value->as.number);
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
            ObjFunction* function = TO_OBJ(ObjFunction, value);
            bytes = (void*)function;
            *byte_length = (int)sizeof(ObjFunction*);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = TO_OBJ(ObjClosure, value);
            bytes = (void*)closure;
            *byte_length = (int)sizeof(ObjClosure*);
            break;
        }
        case OBJ_NATIVE:
        {
            ObjNativeFunction* native = TO_OBJ(ObjNativeFunction, value);
            bytes = (void*)native;
            *byte_length = (int)sizeof(ObjNativeFunction*);
            break;
        }
        case OBJ_TABLE:
        {
            ObjTable* table = TO_OBJ(ObjTable, value);
            bytes = (void*)table;
            *byte_length = (int)sizeof(ObjTable*);
            break;
        }
        case OBJ_UPVALUE:
        {
            ObjUpvalue* upvalue = TO_OBJ(ObjUpvalue, value);
            bytes = (void*)upvalue;
            *byte_length = (int)sizeof(ObjUpvalue*);
            break;
        }
        case NIL:
            *byte_length = 0;
            bytes = NULL;
            break;
    }

    return bytes;
#undef TO_OBJ
}

uint32_t generateHash(const Value *value, HashFn hashFunc)
{
    int value_length;
    void* value_bytes = valueToByte(value, &value_length);
    uint32_t hash = hashFunc(value_bytes, value_length);

    return hash;
}
