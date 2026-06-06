#ifndef common_chunk_value_h
#define common_chunk_value_h

#include <stddef.h>

#define otype(o) (o->type)
#define vtype(v) (v.type)

#define IS_BOOL(v) (vtype(v) == BOOL)
#define IS_NUM(v) (vtype(v) == NUMBER)
#define IS_NIL(v) (vtype(v) == NIL)
#define IS_STRING(v) (vtype(v) == OBJ_STRING)
#define IS_NATIVE(v) (vtype(v) == OBJ_NATIVE)
#define IS_FUNCTION(v) (vtype(v) == OBJ_FUNCTION)
#define IS_CLOSURE(v) (vtype(v) == OBJ_CLOSURE)
#define IS_TABLE(v) (vtype(v) == OBJ_TABLE)
#define IS_UPVALUE(v) (vtype(v) == OBJ_UPVALUE)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.object)
#define AS_CLOSURE(value) ((ObjClosure*)(AS_OBJ(value)))
#define AS_UPVALUE(value) ((ObjUpvalue*)(AS_OBJ(value)))
#define AS_TABLE(value) ((ObjTable*)(AS_OBJ(value)))
#define AS_FUNCTION(value) ((ObjFunction*)(AS_OBJ(value)))
#define AS_NATIVE(value) ((ObjNativeFunction*)(AS_OBJ(value)))
#define AS_STRING(value) ((ObjString*)(AS_OBJ(value)))

#define OBJ_VAL(type, obj) ((Value){ type, { .object = (Object*)(obj) } })
#define STRING_VAL(str) (OBJ_VAL(OBJ_STRING, str))
#define FUNCTION_VAL(f) (OBJ_VAL(OBJ_FUNCTION, f))
#define CLOSURE_VAL(closure) (OBJ_VAL(OBJ_CLOSURE, closure))
#define NATIVE_VAL(native) (OBJ_VAL(OBJ_NATIVE, native))
#define TABLE_VAL(table) (OBJ_VAL(OBJ_TABLE, table))
#define BOOL_VAL(b) ((Value){ BOOL, { .boolean = b } })
#define TRUE_VAL() ((Value){ BOOL, { .boolean = true } })
#define FALSE_VAL() ((Value){ BOOL, { .boolean = false } })
#define NUM_VAL(n) ((Value){ NUMBER, { .number = n } })
#define NIL_VAL() ((Value){ NIL })

#define MAKE_RET(actual) (actual + 1)
#define GET_RET(status) (status - 1)

#define v2obj(type, obj) ((type*)AS_OBJ(obj))

typedef struct Object Object;

typedef enum
{
    NUMBER,
    BOOL,
    NIL,
    OBJ_STRING,
    OBJ_TABLE,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_NATIVE,
    OBJ_UPVALUE
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        double number;
        bool boolean;
        Object* object;
    } as;
} Value;

extern const Value NIL_CONSTANT;

void printValue(Value value);
void printValueNewLine(Value value);
bool compareValue(Value a, Value b);
bool isFalsey(Value value);

typedef struct
{
    // stores the overall capacity
    size_t capacity;
    // stores the index of the next empty position
    size_t count;
    Value* values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

#endif
