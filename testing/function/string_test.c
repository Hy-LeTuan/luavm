#include <table.h>
#include <hash.h>
#include <objstring.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef struct
{
    const char* str;
    int length;
} StringPair;

typedef struct
{
    StringPair a;
    StringPair b;
} ConcatPair;

#define STRING_LITERALS_SIZE 31
StringPair STRING_LITERALS[STRING_LITERALS_SIZE] = {
    [0] = { .str = "", .length = 0 },
    [1] = { .str = "a", .length = 1 },
    [2] = { .str = "ab", .length = 2 },
    [3] = { .str = "abc", .length = 3 },
    [4] = { .str = "abcd", .length = 4 },
    [5] = { .str = "abcde", .length = 5 },
    [6] = { .str = "hello", .length = 5 },
    [7] = { .str = "world", .length = 5 },
    [8] = { .str = "hash", .length = 4 },
    [9] = { .str = "table", .length = 5 },
    [10] = { .str = "test", .length = 4 },
    [11] = { .str = "aaaaaaaaaa", .length = 10 },
    [12] = { .str = "bbbbbbbbbb", .length = 10 },
    [13] = { .str = "ababababab", .length = 10 },
    [14] = { .str = "abcabcabcabc", .length = 12 },
    [15] = { .str = "The quick brown fox", .length = 19 },
    [16] = { .str = "jumps over the lazy dog", .length = 24 },
    [17] = { .str = "The quick brown fox jumps over the lazy dog", .length = 43 },
    [18] = { .str = "1234567890", .length = 10 },
    [19] = { .str = "!@#$%^&*()", .length = 10 },
    [20] = { .str = "mixed123!@#", .length = 12 },
    [21] = { .str = "collision1", .length = 10 },
    [22] = { .str = "collision2", .length = 10 },
    [23] = { .str = "collision3", .length = 10 },
    [24] = { .str = "long_string_with_repeated_patterns_aaaaaaaaaaaaaaaaaaaa", .length = 58 },
    [25] = { .str = "long_string_with_repeated_patterns_bbbbbbbbbbbbbbbbbbbb", .length = 58 },
    [26] = { .str = "case", .length = 4 },
    [27] = { .str = "Case", .length = 4 },
    [28] = { .str = "CASE", .length = 4 },
    [29] = { .str = "   ", .length = 3 },
    [30] = { .str = "\n\t", .length = 2 },
};

#define STRING_CONCAT_SIZE 31
ConcatPair CONCAT_LITERALS[STRING_CONCAT_SIZE] = { [1] = { { .str = "", .length = 0 },
                                                     { .str = "", .length = 0 } },
    [2] = { { .str = "a", .length = 1 }, { .str = "b", .length = 1 } },
    [3] = { { .str = "ab", .length = 2 }, { .str = "cd", .length = 2 } },
    [4] = { { .str = "hello", .length = 5 }, { .str = "world", .length = 5 } },
    [5] = { { .str = "foo", .length = 3 }, { .str = "barbaz", .length = 6 } },
    [6] = { { .str = "aaaaaaaaaa", .length = 10 }, { .str = "bbbbbbbbbb", .length = 10 } },
    [7] = { { .str = "abababab", .length = 8 }, { .str = "cdcdcdcd", .length = 8 } },
    [8] = { { .str = "The quick brown ", .length = 16 }, { .str = "fox jumps", .length = 9 } },
    [9] = { { .str = "jumps over ", .length = 11 }, { .str = "the lazy dog", .length = 12 } },
    [10] = { { .str = "12345", .length = 5 }, { .str = "67890", .length = 5 } },
    [11] = { { .str = "!@#$%", .length = 5 }, { .str = "^&*()", .length = 5 } },
    [12] = { { .str = "case", .length = 4 }, { .str = "CASE", .length = 4 } },
    [13] = { { .str = "   ", .length = 3 }, { .str = "   ", .length = 3 } },
    [14] = { { .str = "\n\t", .length = 2 }, { .str = "abc", .length = 3 } },
    [15] = { { .str = "prefix_", .length = 7 }, { .str = "suffix", .length = 6 } },
    [16] = { { .str = "long_string_", .length = 12 },
      { .str = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", .length = 50 } },
    [17] = {
      { .str = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
               "yyyyyyyyyyyyyyyyyyyyyyy",
        .length = 100 },
      { .str = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
               "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
               "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
        .length = 200 } } };

static void internTest(Table* strings)
{
    // insert all strings
    ObjString* literalControls[STRING_LITERALS_SIZE];
    for (int i = 0; i < STRING_LITERALS_SIZE; i++)
    {
        StringPair* pair = &STRING_LITERALS[i];
        ObjString* control = copyString(pair->str, pair->length, strings);
        literalControls[i] = control;
    }

    // check if they are interned
    for (int i = 0; i < STRING_LITERALS_SIZE; i++)
    {
        StringPair* pair = &STRING_LITERALS[i];
        ObjString* control = literalControls[i];

        ObjString* test = copyString(pair->str, pair->length, strings);

        if (control != test)
        {
            fprintf(stderr, "Intern test failed at pair: %d, with literal '%s'.\n", i, pair->str);
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stdout, "Intern test passed successfully.\n");
}

static void concateTest(Table* strings)
{
    ObjString* literalControls[STRING_CONCAT_SIZE];
    for (int i = 0; i < STRING_CONCAT_SIZE; i++)
    {
        ConcatPair* pair = &CONCAT_LITERALS[i];
        ObjString* joined = concatenateString(copyString(pair->a.str, pair->a.length, strings),
          copyString(pair->b.str, pair->b.length, strings), strings);
        literalControls[i] = joined;
    }

    for (int i = 0; i < STRING_CONCAT_SIZE; i++)
    {
        ConcatPair* pair = &CONCAT_LITERALS[i];
        ObjString* control = literalControls[i];

        size_t length = pair->a.length + pair->b.length;
        char* chars = malloc(length + 1);
        memcpy(chars, pair->a.str, pair->a.length);
        memcpy(chars + pair->a.length, pair->b.str, pair->b.length);
        chars[length] = '\0';

        ObjString* test = takeString(chars, length, strings);

        if (control != test)
        {
            fprintf(stderr,
              "Concat test failed at pair: %d, with literals '%s' and '%s'. Duplication found.\n",
              i, pair->a.str, pair->b.str);
            exit(EXIT_FAILURE);
        }
        else if (control->length != test->length ||
          memcmp(control->chars, test->chars, control->length) != 0)
        {
            fprintf(stderr,
              "Concat test failed at pair: %d, with literals '%s' and '%s'. Concat result is "
              "different.\n",
              i, pair->a.str, pair->b.str);
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stdout, "Concat test passed successfully.\n");
}

int main(int argc, char* argv[])
{
    Table strings;
    initTable(&strings);

    internTest(&strings);
    concateTest(&strings);

    freeTable(&strings);

    return 0;
}
