#include <vmstate.h>
#include <object.h>
#include <table.h>
#include <vmdo.h>

#include <stdio.h>
#include <assert.h>

#define CREATE_STRING(str, length, strings) (STRING_VAL(copyString(str, length, &strings)))

int main(int argc, char* argv[])
{
    VM vm;
    initVM(&vm);

    Table test_table;
    initTable(&test_table);

    // create all keys

    Value functionKey = FUNCTION_VAL(newFunction(NULL, &vm));
    Value closureKey = CLOSURE_VAL(newClosure(newFunction(NULL, &vm), &vm));

    Value helloKey = CREATE_STRING("hello", 5, vm);
    Value keyKey = CREATE_STRING("key", 3, vm);
    Value normalKey = CREATE_STRING("normal", 6, vm);
    Value longKey = CREATE_STRING("a very long key", 15, vm);

    // test insertion
    tableSet(helloKey, NUM_VAL(1), &test_table, &vm);
    tableSet(keyKey, NUM_VAL(2), &test_table, &vm);
    tableSet(normalKey, NUM_VAL(3), &test_table, &vm);
    tableSet(longKey, NUM_VAL(122.2), &test_table, &vm);
    tableSet(functionKey, NUM_VAL(1), &test_table, &vm);
    tableSet(closureKey, NUM_VAL(2), &test_table, &vm);

    assert(IS_NUM(tableGet(&helloKey, &test_table)) == 1.0);
    assert(IS_NUM(tableGet(&keyKey, &test_table)) == 2.0);
    assert(IS_NUM(tableGet(&normalKey, &test_table)) == 3.0);
    assert(IS_NUM(tableGet(&longKey, &test_table)) == 122.2);
    assert(IS_NUM(tableGet(&functionKey, &test_table)) == 1.0);
    assert(IS_NUM(tableGet(&closureKey, &test_table)) == 2.0);

    fprintf(stdout, "Insertion test passed successfully.\n");

    // test deletion
    assert(tableErase(&helloKey, &test_table));
    assert(IS_NUM(tableGet(&keyKey, &test_table)) == 2.0);
    assert(IS_NUM(tableGet(&normalKey, &test_table)) == 3.0);
    assert(IS_NUM(tableGet(&longKey, &test_table)) == 122.2);
    assert(IS_NUM(tableGet(&functionKey, &test_table)) == 1.0);
    assert(IS_NUM(tableGet(&closureKey, &test_table)) == 2.0);

    fprintf(stdout, "Deletion test passed successfully.\n");

    // testing set
    tableInsertOrSet(normalKey, NUM_VAL(12), &test_table, &vm);
    assert(IS_NUM(tableGet(&normalKey, &test_table)) == 12.0);

    tableInsertOrSet(longKey, NUM_VAL(122.5), &test_table, &vm);
    assert(IS_NUM(tableGet(&longKey, &test_table)) == 122.5);

    fprintf(stdout, "Set/Insert test passed successfully.\n");

    freeTable(&test_table, &vm);
    freeTable(&vm.strings, &vm);

    return 0;
}
