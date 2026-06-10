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

void initTable(Table* table);
void tableSet(Value key, Value value, Table* table);
void tableInsertOrSet(Value key, Value value, Table* table);
Value tableGet(Value key, Table* table);
Value tableGetWithPtr(const char* c, int l, Table* t);
bool tableErase(Value key, Table* table);
void freeTable(Table* table);

Value tableFindString(const char* c, int l, Table* t);

#endif
