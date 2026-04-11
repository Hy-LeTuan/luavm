#include <table.h>
#include <object.h>

#include <stdio.h>
#include <assert.h>

#define CREATE_STRING(str, length) (OBJ_VAL((Object*)copyString(str, length)))

int main(int argc, char* argv[])
{
    Table table;
    initTable(&table);

    insert(CREATE_STRING("hello", 5), NUM_VAL(1), &table);
    insert(CREATE_STRING("key", 3), NUM_VAL(2), &table);
    insert(CREATE_STRING("normal", 6), NUM_VAL(3), &table);

    Value a = get(CREATE_STRING("hello", 5), &table);
    printf("value of key 'hello': ");
    printValue(a);
    printf("\n");
    assert(a.as.number == 1.0);

    Value b = get(CREATE_STRING("key", 3), &table);
    printf("value of key 'key': ");
    printValue(b);
    printf("\n");
    assert(b.as.number == 2.0);

    Value c = get(CREATE_STRING("normal", 6), &table);
    printf("value of key 'normal': ");
    printValue(c);
    printf("\n");
    assert(c.as.number == 3.0);

    printf("erasing 'hello'\n");
    assert(erase(CREATE_STRING("hello", 5), &table));

    printf("value of key 'key' after deleting 'hello': ");
    printValue(get(CREATE_STRING("key", 3), &table));
    printf("\n");
    assert(get(CREATE_STRING("key", 3), &table).as.number == 2.0);

    printf("value of key 'normal' after deleting 'hello': ");
    printValue(get(CREATE_STRING("normal", 6), &table));
    printf("\n");
    assert(get(CREATE_STRING("normal", 6), &table).as.number == 3.0);

    freeTable(&table);

    return 0;
}
