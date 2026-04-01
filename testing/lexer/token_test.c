#include <file_utils.h>
#include <lexer.h>

#include <assert.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    const char* source = readSourceFile(argv[1]);

    Lexer lexer;
    initLexer(source, &lexer);

    Token tokens[26] = { [0] = { .type = TOKEN_EQUAL },
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
        [25] = { .type = TOKEN_THREE_DOTS } };

    int i = 0;

    bool hadError = false;

    for (int i = 0; i < 26; i++)
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
