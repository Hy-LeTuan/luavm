#ifndef common_table_table_h
#define common_table_table_h

#include <value.h>

typedef struct VM VM;

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

// if i include VM here, this would mean that compilers will need the VM taken from the active
// parser to function

void initTable(Table* table);
void tableSet(Value key, Value value, Table* table, VM* vm);
void tableInsertOrSet(Value key, Value value, Table* table, VM* vm);
Value* tableGet(const Value* key, Table* table);
Value* tableGetWithPtr(const char* c, int l, Table* t);
bool tableErase(Value* key, Table* table);
void freeTable(Table* table, VM* vm);

Value tableFindString(const char* c, int l, Table* t);

#endif
