#include <table.h>

#include <hash.h>
#include <memory.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table)
{
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void* valueToByte(Value value, int* byte_length)
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
                case OBJ_TABLE:
                case OBJ_FUNCTION:
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

static bool compareKey(Value a, Value b)
{
    if (a.type != b.type)
    {
        return false;
    }
    if (IS_BOOL(a) && IS_BOOL(b))
    {
        return AS_BOOL(a) == AS_BOOL(b);
    }
    else if (IS_NUM(a) && IS_NUM(b))
    {
        return AS_NUM(a) == AS_NUM(b);
    }
    else if (IS_STRING(a) == IS_STRING(b))
    {
        ObjString* a_str = AS_STRING(a);
        ObjString* b_str = AS_STRING(b);

        return a_str->length == b_str->length &&
          memcmp(a_str->chars, b_str->chars, a_str->length) == 0;
    }

    return false;
}

static Entry* findEntry(Entry* entries, size_t capacity, Value key)
{
    int len = 0;
    uint32_t hash = fnv1a_32(valueToByte(key, &len), len);
    int index = hash % capacity;

    uint32_t iteration = 0;
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

static int growTable(Table* table, size_t newCapacity)
{
    Entry* newEntries = ALLOCATE(Entry, sizeof(Entry) * newCapacity);

    for (int i = 0; i < newCapacity; i++)
    {
        Entry* entry = &newEntries[i];
        entry->key = NIL_VAL();
        entry->value = NIL_VAL();
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
            table->count++;
        }
    }

    FREE_ARRAY(table->entries, table->capacity, Entry);
    table->entries = newEntries;
    table->capacity = newCapacity;

    return 0;
}

void insert(Value key, Value value, Table* table)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        size_t newCapacity = GROW_SIZE(table->capacity);
        growTable(table, newCapacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry != NULL)
    {
        entry->type = ENTRY_OCCUPIED;
        entry->key = key;
        entry->value = value;
    }
}

Value get(Value key, Table* table)
{
    Entry* entry = findEntry(table->entries, table->capacity, key);

    if (entry != NULL && entry->type == ENTRY_OCCUPIED)
    {
        return entry->value;
    }

    return NIL_VAL();
}

bool erase(Value key, Table* table)
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

void freeTable(Table* table)
{
    FREE_ARRAY(table->entries, table->capacity, Entry);
    table->count = 0;
    table->capacity = 0;
}
