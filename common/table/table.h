#ifndef common_table_table_h
#define common_table_table_h

#include <value.h>
#include <stdint.h>

typedef enum
{
    ENTRY_EMPTY,
    ENTRY_TOMBSTONE,
    ENTRY_OCCUPIED
} EntryType;

typedef struct
{
    Value value;
    Value key;
    EntryType type;
} Entry;

typedef struct
{
    size_t count;
    size_t capacity;
    Entry* entries;
} Table;

void* valueToByte(Value value, int* byte_length);

void initTable(Table* table);
void insert(Value key, Value value, Table* table);
Value get(Value key, Table* table);
bool erase(Value key, Table* table);
void freeTable(Table* table);

#endif
