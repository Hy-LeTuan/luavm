#include <table.h>
#include <object.h>
#include <objstring.h>
#include <objfunction.h>
#include <objclosure.h>

#include <stdio.h>
#include <assert.h>

#define CREATE_STRING(str, length, strings) (OBJ_VAL((Object*)copyString(str, length, &strings)))

int main(int argc, char* argv[])
{
    Table test_table;
    Table strings;

    initTable(&test_table);
    initTable(&strings);

    // test insertion
    tableInsert(CREATE_STRING("hello", 5, strings), NUM_VAL(1), &test_table);
    tableInsert(CREATE_STRING("key", 3, strings), NUM_VAL(2), &test_table);
    tableInsert(CREATE_STRING("normal", 6, strings), NUM_VAL(3), &test_table);
    tableInsert(CREATE_STRING("a very long key", 15, strings), NUM_VAL(122.2), &test_table);

    ObjFunction* function = newFunction();
    tableInsert(OBJ_VAL((Object*)function), NUM_VAL(1), &test_table);

    ObjClosure* closure = newClosure(function);
    tableInsert(OBJ_VAL((Object*)closure), NUM_VAL(2), &test_table);

    assert(AS_NUM(tableGet(CREATE_STRING("hello", 5, strings), &test_table)) == 1.0);
    assert(AS_NUM(tableGet(CREATE_STRING("key", 3, strings), &test_table)) == 2.0);
    assert(AS_NUM(tableGet(CREATE_STRING("normal", 6, strings), &test_table)) == 3.0);
    assert(AS_NUM(tableGet(CREATE_STRING("a very long key", 15, strings), &test_table)) == 122.2);
    assert(AS_NUM(tableGet(OBJ_VAL((Object*)function), &test_table)) == 1.0);
    assert(AS_NUM(tableGet(OBJ_VAL((Object*)closure), &test_table)) == 2.0);

    fprintf(stdout, "Insertion test passed successfully.\n");

    // test deletion
    assert(tableErase(CREATE_STRING("hello", 5, strings), &test_table));
    assert(AS_NUM(tableGet(CREATE_STRING("key", 3, strings), &test_table)) == 2.0);
    assert(AS_NUM(tableGet(CREATE_STRING("normal", 6, strings), &test_table)) == 3.0);
    assert(AS_NUM(tableGet(CREATE_STRING("a very long key", 15, strings), &test_table)) == 122.2);

    fprintf(stdout, "Deletion test passed successfully.\n");

    // testing set
    tableInsertOrSet(CREATE_STRING("normal", 6, strings), NUM_VAL(12), &test_table);
    assert(AS_NUM(tableGet(CREATE_STRING("normal", 6, strings), &test_table)) == 12.0);

    tableInsertOrSet(CREATE_STRING("a very long key", 15, strings), NUM_VAL(122.5), &test_table);
    assert(AS_NUM(tableGet(CREATE_STRING("a very long key", 15, strings), &test_table)) == 122.5);

    fprintf(stdout, "Set/Insert test passed successfully.\n");

    freeTable(&test_table);
    freeTable(&strings);

    return 0;
}
