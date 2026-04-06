#ifndef common_chunk_value_h
#define common_chunk_value_h

#include <stddef.h>

#define IS_BOOL(value) ((value).type == BOOL)
#define IS_NUM(value) ((value).type == NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUM(value) ((value).as.number)

#define BOOL_VALUE(b) ((Value){ BOOL, { .boolean = b } })
#define NUM_VAL(n) ((Value){ NUMBER, { .number = n } })

typedef enum
{
    NUMBER,
    BOOL
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        double number;
        bool boolean;
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

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value* value);
void printValueNewLine(Value* value);

#endif
