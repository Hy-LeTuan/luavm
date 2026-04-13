#ifndef common_chunk_value_h
#define common_chunk_value_h

#include <stddef.h>

#define IS_BOOL(value) ((value).type == BOOL)
#define IS_NUM(value) ((value).type == NUMBER)
#define IS_NIL(value) ((value).type == NIL)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)

#define BOOL_VAL(b) ((Value){ BOOL, { .boolean = b } })
#define NUM_VAL(n) ((Value){ NUMBER, { .number = n } })
#define NIL_VAL() ((Value){ NIL })

typedef struct Object Object;

typedef enum
{
    NUMBER,
    BOOL,
    NIL,
    OBJECT
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

typedef struct
{
    // stores the overall capacity
    size_t capacity;
    // stores the index of the next empty position
    size_t count;
    Value* values;
} ValueArray;

extern const Value NIL_CONSTANT;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

void printValue(Value value);
void printValueNewLine(Value value);

#endif
