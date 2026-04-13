#include <table.h>
#include <object.h>
#include <objstring.h>

#include <stdio.h>
#include <assert.h>

Table strings;
#define CREATE_STRING(str, length, strings) (OBJ_VAL((Object*)copyString(str, length, &strings)))

int main(int argc, char* argv[])
{
    Table table;
    initTable(&table);

    tableInsert(CREATE_STRING("hello", 5, strings), NUM_VAL(1), &table);
    tableInsert(CREATE_STRING("key", 3, strings), NUM_VAL(2), &table);
    tableInsert(CREATE_STRING("normal", 6, strings), NUM_VAL(3), &table);

    Value a = tableGet(CREATE_STRING("hello", 5, strings), &table);
    printf("value of key 'hello': ");
    printValue(a);
    printf("\n");
    assert(a.as.number == 1.0);

    Value b = tableGet(CREATE_STRING("key", 3, strings), &table);
    printf("value of key 'key': ");
    printValue(b);
    printf("\n");
    assert(b.as.number == 2.0);

    Value c = tableGet(CREATE_STRING("normal", 6, strings), &table);
    printf("value of key 'normal': ");
    printValue(c);
    printf("\n");
    assert(c.as.number == 3.0);

    printf("erasing 'hello'\n");
    assert(tableErase(CREATE_STRING("hello", 5, strings), &table));

    printf("value of key 'key' after deleting 'hello': ");
    printValue(tableGet(CREATE_STRING("key", 3, strings), &table));
    printf("\n");
    assert(tableGet(CREATE_STRING("key", 3, strings), &table).as.number == 2.0);

    printf("value of key 'normal' after deleting 'hello': ");
    printValue(tableGet(CREATE_STRING("normal", 6, strings), &table));
    printf("\n");
    assert(tableGet(CREATE_STRING("normal", 6, strings), &table).as.number == 3.0);

    freeTable(&table);

    return 0;
}
