#include <object.h>
#include <table.h>

#include <stdio.h>
#include <assert.h>

#define CREATE_STRING(str, length, strings) (STRING_VAL(copyString(str, length, &strings)))

int main(int argc, char* argv[])
{
    Table test_table;
    Table strings;

    initTable(&test_table);
    initTable(&strings);

    // test insertion
    tableSet(CREATE_STRING("hello", 5, strings), NUM_VAL(1), &test_table);
    tableSet(CREATE_STRING("key", 3, strings), NUM_VAL(2), &test_table);
    tableSet(CREATE_STRING("normal", 6, strings), NUM_VAL(3), &test_table);
    tableSet(CREATE_STRING("a very long key", 15, strings), NUM_VAL(122.2), &test_table);

    ObjFunction* function = newFunction();
    tableSet(FUNCTION_VAL(function), NUM_VAL(1), &test_table);

    ObjClosure* closure = newClosure(function);
    tableSet(CLOSURE_VAL(closure), NUM_VAL(2), &test_table);

    assert(AS_NUM(tableGet(CREATE_STRING("hello", 5, strings), &test_table)) == 1.0);
    assert(AS_NUM(tableGet(CREATE_STRING("key", 3, strings), &test_table)) == 2.0);
    assert(AS_NUM(tableGet(CREATE_STRING("normal", 6, strings), &test_table)) == 3.0);
    assert(AS_NUM(tableGet(CREATE_STRING("a very long key", 15, strings), &test_table)) == 122.2);
    assert(AS_NUM(tableGet(FUNCTION_VAL(function), &test_table)) == 1.0);
    assert(AS_NUM(tableGet(CLOSURE_VAL(closure), &test_table)) == 2.0);

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
