#include <table.h>

#include <hash.h>
#include <memory.h>
#include <string.h>
#include <object.h>
#include <objstring.h>
#include <objfunction.h>
#include <objclosure.h>
#include <objnativefunction.h>

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table)
{
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

static bool compareKey(Value a, Value b)
{
    if (a.type != b.type)
    {
        return false;
    }
    else if (IS_NIL(a))
    {
        return false;
    }
    else if (IS_BOOL(a))
    {
        return AS_BOOL(a) == AS_BOOL(b);
    }
    else if (IS_NUM(a))
    {
        return AS_NUM(a) == AS_NUM(b);
    }
    else if (IS_STRING(a))
    {
        return AS_STRING(a) == AS_STRING(b);
    }
    else if (IS_FUNCTION(a))
    {
        return AS_FUNCTION(a) == AS_FUNCTION(b);
    }
    else if (IS_CLOSURE(a))
    {
        return AS_CLOSURE(a) == AS_CLOSURE(b);
    }
    else if (IS_NATIVE(a))
    {
        return AS_NATIVE(a) == AS_NATIVE(b);
    }

    return false;
}

/*
 * Find either the occupied entry or the closest empty entry based on the provided key.
 * Returns: NULL when capacity is 0, an Entry otherwise.
 * */
static Entry* findEntry(Entry* entries, size_t capacity, Value key)
{
    if (capacity == 0)
    {
        return NULL;
    }

    uint32_t hash = generateHash(key, fnv1a_32);
    int index = hash % capacity;

    int iteration = 0;
    Entry* tombstone = NULL;

    while (1)
    {
        Entry* entry = &entries[index];

        if (entry->type == ENTRY_EMPTY)
        {
            return tombstone == NULL ? entry : tombstone;
        }
        else if (entry->type == ENTRY_TOMBSTONE)
        {
            tombstone = entry;
        }
        else if (compareKey(entry->key, key))
        {
            return entry;
        }

        // mid square probing
        index = (index + iteration * iteration) % capacity;
        iteration++;
    }

    return NULL;
}

static int tableGrow(Table* table, size_t newCapacity)
{
    Entry* newEntries = ALLOCATE(Entry, sizeof(Entry) * newCapacity);

    for (int i = 0; i < newCapacity; i++)
    {
        Entry* entry = &newEntries[i];
        entry->key = NIL_CONSTANT;
        entry->value = NIL_CONSTANT;
        entry->type = ENTRY_EMPTY;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];

        if (entry->type == ENTRY_EMPTY || entry->type == ENTRY_TOMBSTONE)
        {
            continue;
        }

        Entry* to = findEntry(newEntries, newCapacity, entry->key);

        if (to != NULL)
        {
            to->key = entry->key;
            to->value = entry->value;
            to->type = ENTRY_OCCUPIED;
            table->count++;
        }
    }

    FREE_ARRAY(table->entries, table->capacity, Entry);
    table->entries = newEntries;
    table->capacity = newCapacity;

    return 0;
}

void tableInsert(Value key, Value value, Table* table)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        size_t newCapacity = GROW_SIZE(table->capacity);
        tableGrow(table, newCapacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry != NULL)
    {
        entry->type = ENTRY_OCCUPIED;
        entry->key = key;
        entry->value = value;
        table->count++;
    }
}

void tableInsertOrSet(Value key, Value value, Table* table)
{
    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry != NULL && entry->type == ENTRY_OCCUPIED)
    {
        entry->value = value;
    }
    else
    {
        tableInsert(key, value, table);
    }
}

Value tableGet(Value key, Table* table)
{
    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry != NULL && entry->type == ENTRY_OCCUPIED)
    {
        return entry->value;
    }

    return NIL_CONSTANT;
}

bool tableErase(Value key, Table* table)
{
    if (IS_NIL(key))
    {
        return false;
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry == NULL || entry->type != ENTRY_OCCUPIED)
    {
        return false;
    }

    entry->value = NIL_VAL();
    entry->type = ENTRY_TOMBSTONE;

    return true;
}

Value tableFindString(const char* chars, int length, Table* table)
{
    if (table->count == 0)
    {
        return NIL_CONSTANT;
    }

    uint32_t hash = fnv1a_32(chars, length);
    int index = hash % table->capacity;
    int iteration = 0;

    while (1)
    {
        Entry* entry = &table->entries[index];

        if (entry->type == ENTRY_EMPTY)
        {
            return NIL_CONSTANT;
        }
        else if (entry->type == ENTRY_OCCUPIED && IS_STRING(entry->key))
        {
            ObjString* key = AS_STRING(entry->key);
            if (key->length == length && memcmp(chars, key->chars, length) == 0)
            {
                return entry->key;
            }
        }

        // mid square probing
        index = (index + iteration * iteration) % table->capacity;
        iteration++;
    }

    return NIL_CONSTANT;
}

void freeTable(Table* table)
{
    FREE_ARRAY(table->entries, table->capacity, Entry);
    table->count = 0;
    table->capacity = 0;
}
