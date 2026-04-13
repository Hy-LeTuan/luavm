#include <table.h>
#include <hash.h>
#include <objstring.h>

#include <stdio.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    Table strings;
    initTable(&strings);

    ObjString* a = copyString("hello", 5, &strings);
    ObjString* a_duplicate = copyString("hello", 5, &strings);

    ObjString* b = copyString("this is a long string", 21, &strings);
    ObjString* b_duplicate = copyString("this is a long string", 21, &strings);

    assert(a == a_duplicate);
    assert(b == b_duplicate);

    fprintf(stdout, "comparison test completed.\n");

    ObjString* op1 = copyString("hello, ", 7, &strings);
    ObjString* op2 = copyString("world", 5, &strings);
    ObjString* result = copyString("hello, world", 12, &strings);

    assert(result == concatenateString(op1, op2, &strings));

    fprintf(stdout, "concatenation test completed.\n");

    freeTable(&strings);

    return 0;
}
