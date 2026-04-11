#include <table.h>
#include <hash.h>
#include <value.h>

#include <stdio.h>
#include <assert.h>

#define COMPARE(expr) (compare((expr), (expr)))

static void compare(Value a, Value b)
{

    int a_hash_length;
    int b_hash_length;

    void* a_bytes = valueToByte(a, &a_hash_length);
    void* b_bytes = valueToByte(b, &b_hash_length);

    uint32_t a_hash = fnv1a_32(a_bytes, a_hash_length);
    uint32_t b_hash = fnv1a_32(b_bytes, b_hash_length);

    fprintf(stdout, "a_hash: %u\n", a_hash);
    fprintf(stdout, "b_hash: %u\n", a_hash);

    assert(a_hash == b_hash);
}

int main(int argc, char* argv[])
{
    // numbers
    COMPARE(NUM_VAL(1.0));
    COMPARE(NUM_VAL(2.0));
    COMPARE(NUM_VAL(3.0));
    COMPARE(NUM_VAL(12888888.0));
    COMPARE(NUM_VAL(12888888.5));

    fprintf(stdout, "Test for numbers passed.\n");

    // booleans
    COMPARE(BOOL_VAL(true));
    COMPARE(BOOL_VAL(false));

    fprintf(stdout, "Test for booleans passed.\n");

    // strings
    COMPARE(OBJ_VAL((Object*)copyString("hello", 5)));
    COMPARE(OBJ_VAL((Object*)copyString("this is a very long string for a key", 36)));

    fprintf(stdout, "Test for strings passed.\n");

    return 0;
}
