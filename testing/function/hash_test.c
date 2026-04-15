#include <table.h>
#include <hash.h>
#include <value.h>
#include <objstring.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>

#define COMPARE(expr, func) (compare((expr), (expr), func))

Table strings;

static void compare(Value a, Value b, HashFn hashFunc)
{
    int64_t a_hash = generateHash(a, hashFunc);
    int64_t b_hash = generateHash(b, hashFunc);

    fprintf(stdout, "a hash is: %ld, and b hash is: %ld\n", a_hash, b_hash);

    assert(a_hash >= 0);
    assert(b_hash >= 0);
    assert(a_hash == b_hash);
}

int main(int argc, char* argv[])
{
    initTable(&strings);

    // numbers
    COMPARE(NUM_VAL(1.0), fnv1a_32);
    COMPARE(NUM_VAL(2.0), fnv1a_32);
    COMPARE(NUM_VAL(INT_MAX), fnv1a_32);
    COMPARE(NUM_VAL(12888888.0), fnv1a_32);
    COMPARE(NUM_VAL(12888888.5), fnv1a_32);

    COMPARE(NUM_VAL(-1.0), fnv1a_32);
    COMPARE(NUM_VAL(INT_MIN), fnv1a_32);
    COMPARE(NUM_VAL(-12888888.0), fnv1a_32);
    COMPARE(NUM_VAL(-12888888.5), fnv1a_32);

    fprintf(stdout, "Test for numbers passed.\n");

    // booleans
    COMPARE(BOOL_VAL(true), fnv1a_32);
    COMPARE(BOOL_VAL(false), fnv1a_32);

    fprintf(stdout, "Test for booleans passed.\n");

    // strings
    COMPARE(OBJ_VAL((Object*)copyString("hello", 5, &strings)), fnv1a_32);
    COMPARE(
      OBJ_VAL((Object*)copyString("this is a very long string for a key", 36, &strings)), fnv1a_32);

    fprintf(stdout, "Test for strings passed.\n");

    freeTable(&strings);

    return 0;
}
