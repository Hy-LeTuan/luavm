#ifndef common_table_table_h
#define common_table_table_h

#include <value.h>
#include <object.h>
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

void initTable(Table* table);
void tableInsert(Value key, Value value, Table* table);
void tableInsertOrSet(Value key, Value value, Table* table);
Value tableGet(Value key, Table* table);
bool tableErase(Value key, Table* table);
void freeTable(Table* table);

Value tableFindString(const char* chars, int length, Table* table);

#endif
