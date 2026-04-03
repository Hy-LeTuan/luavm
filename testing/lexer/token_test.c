#include "token.h"
#include <file_utils.h>
#include <lexer.h>

#include <assert.h>
#include <stdio.h>

#define TOKEN_TEST_SIZE 71

int main(int argc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);

    Lexer lexer;
    initLexer(source, &lexer);

    Token tokens[TOKEN_TEST_SIZE] = {
        [0] = { .type = TOKEN_EQUAL },
        [1] = { .type = TOKEN_PLUS },
        [2] = { .type = TOKEN_MINUS },
        [3] = { .type = TOKEN_STAR },
        [4] = { .type = TOKEN_SLASH },
        [5] = { .type = TOKEN_PERCENT },
        [6] = { .type = TOKEN_CARET },
        [7] = { .type = TOKEN_HASH },
        [8] = { .type = TOKEN_LESS },
        [9] = { .type = TOKEN_GREATER },
        [10] = { .type = TOKEN_LEFT_PAREN },
        [11] = { .type = TOKEN_RIGHT_PAREN },
        [12] = { .type = TOKEN_LEFT_BRACE },
        [13] = { .type = TOKEN_RIGHT_BRACE },
        [14] = { .type = TOKEN_LEFT_SQUARE },
        [15] = { .type = TOKEN_RIGHT_SQUARE },
        [16] = { .type = TOKEN_SEMICOLON },
        [17] = { .type = TOKEN_COLON },
        [18] = { .type = TOKEN_COMMA },
        [19] = { .type = TOKEN_DOT },
        [20] = { .type = TOKEN_EQUAL_EQUAL },
        [21] = { .type = TOKEN_TILDE_EQUAL },
        [22] = { .type = TOKEN_GREATER_EQUAL },
        [23] = { .type = TOKEN_LESS_EQUAL },
        [24] = { .type = TOKEN_DOT_DOT },
        [25] = { .type = TOKEN_THREE_DOTS },
        // keywords
        [26] = { .type = TOKEN_AND },
        [27] = { .type = TOKEN_BREAK },
        [28] = { .type = TOKEN_DO },
        [29] = { .type = TOKEN_ELSE },
        [30] = { .type = TOKEN_ELSEIF },
        [31] = { .type = TOKEN_END },
        [32] = { .type = TOKEN_FALSE },
        [33] = { .type = TOKEN_FOR },
        [34] = { .type = TOKEN_FUNCTION },
        [35] = { .type = TOKEN_IF },
        [36] = { .type = TOKEN_IN },
        [37] = { .type = TOKEN_LOCAL },
        [38] = { .type = TOKEN_NOT },
        [39] = { .type = TOKEN_OR },
        [40] = { .type = TOKEN_REPEAT },
        [41] = { .type = TOKEN_RETURN },
        [42] = { .type = TOKEN_THEN },
        [43] = { .type = TOKEN_TRUE },
        [44] = { .type = TOKEN_UNTIL },
        [45] = { .type = TOKEN_WHILE },
        // identifiers
        [46] = { .type = TOKEN_IDENTIFIER },
        [47] = { .type = TOKEN_IDENTIFIER },
        [48] = { .type = TOKEN_IDENTIFIER },
        [49] = { .type = TOKEN_IDENTIFIER },
        [50] = { .type = TOKEN_IDENTIFIER },
        [51] = { .type = TOKEN_IDENTIFIER },
        [52] = { .type = TOKEN_IDENTIFIER },
        [53] = { .type = TOKEN_IDENTIFIER },
        [54] = { .type = TOKEN_IDENTIFIER },
        [55] = { .type = TOKEN_IDENTIFIER },
        [56] = { .type = TOKEN_IDENTIFIER },
        [57] = { .type = TOKEN_IDENTIFIER },
        [58] = { .type = TOKEN_IDENTIFIER },
        [59] = { .type = TOKEN_IDENTIFIER },
        [60] = { .type = TOKEN_IDENTIFIER },
        [61] = { .type = TOKEN_IDENTIFIER },
        [62] = { .type = TOKEN_IDENTIFIER },
        // string
        [63] = { .type = TOKEN_STRING },
        [64] = { .type = TOKEN_STRING },
        [65] = { .type = TOKEN_STRING },
        [66] = { .type = TOKEN_STRING },
        // numbers
        [67] = { .type = TOKEN_NUMBER },
        [68] = { .type = TOKEN_NUMBER },
        [69] = { .type = TOKEN_NUMBER },
        [70] = { .type = TOKEN_NUMBER },
    };

    int i = 0;
    bool hadError = false;

    for (int i = 0; i < (size_t)TOKEN_TEST_SIZE; i++)
    {
        Token token = lex(&lexer);

        if (token.type == TOKEN_ERROR)
        {
            hadError = true;
            fprintf(stderr, "Test failed on token #%d. [TOKEN_ERROR] encountered.\n", i);
        }
        else if (token.type != tokens[i].type)
        {
            hadError = true;
            fprintf(stderr, "Test failed on token #%d. Expected token: '%d', but got '%d'.\n", i,
              tokens[i].type, token.type);
        }

#ifdef DEBUG_PRINT_TOKEN
        printf("%s\n", convertToken(&token));
#endif
    }

    return hadError ? 1 : 0;
}
