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

    // create all keys

    Value functionKey = FUNCTION_VAL(newFunction());
    Value closureKey = CLOSURE_VAL(newClosure(newFunction()));

    Value helloKey = CREATE_STRING("hello", 5, strings);
    Value keyKey = CREATE_STRING("key", 3, strings);
    Value normalKey = CREATE_STRING("normal", 6, strings);
    Value longKey = CREATE_STRING("a very long key", 15, strings);

    // test insertion
    tableSet(helloKey, NUM_VAL(1), &test_table);
    tableSet(keyKey, NUM_VAL(2), &test_table);
    tableSet(normalKey, NUM_VAL(3), &test_table);
    tableSet(longKey, NUM_VAL(122.2), &test_table);
    tableSet(functionKey, NUM_VAL(1), &test_table);
    tableSet(closureKey, NUM_VAL(2), &test_table);

    assert(tableGet(&helloKey, &test_table).as.number == 1.0);
    assert(tableGet(&keyKey, &test_table).as.number == 2.0);
    assert(tableGet(&normalKey, &test_table).as.number == 3.0);
    assert(tableGet(&longKey, &test_table).as.number == 122.2);
    assert(tableGet(&functionKey, &test_table).as.number == 1.0);
    assert(tableGet(&closureKey, &test_table).as.number == 2.0);

    fprintf(stdout, "Insertion test passed successfully.\n");

    // test deletion
    assert(tableErase(&helloKey, &test_table));
    assert(tableGet(&keyKey, &test_table).as.number == 2.0);
    assert(tableGet(&normalKey, &test_table).as.number == 3.0);
    assert(tableGet(&longKey, &test_table).as.number == 122.2);
    assert(tableGet(&functionKey, &test_table).as.number == 1.0);
    assert(tableGet(&closureKey, &test_table).as.number == 2.0);

    fprintf(stdout, "Deletion test passed successfully.\n");

    // testing set
    tableInsertOrSet(normalKey, NUM_VAL(12), &test_table);
    assert(tableGet(&normalKey, &test_table).as.number == 12.0);

    tableInsertOrSet(longKey, NUM_VAL(122.5), &test_table);
    assert(tableGet(&longKey, &test_table).as.number == 122.5);

    fprintf(stdout, "Set/Insert test passed successfully.\n");

    freeTable(&test_table);
    freeTable(&strings);

    return 0;
}
