#include <compiler.h>
#include <lexer.h>

#include <stdio.h>

static void initParser(Parser* parser)
{
    parser->hadError = false;
}

static void advance(Parser* parser, Lexer* lexer)
{
    parser->prev = parser->current;

    for (;;)
    {
        Token token = lex(lexer);

        if (token.type == TOKEN_ERROR)
        {
            parser->hadError = true;
            fprintf(stderr, "Error at line: %zu\n", token.line);

            return;
        }

        parser->current = token;
        break;
    }
}

void compile(const char* source)
{
    Lexer lexer;
    initLexer(source, &lexer);

    Parser parser;
    initParser(&parser);

    advance(&parser, &lexer);

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.hadError)
        {
            fprintf(stderr, "[TOKEN_ERROR] encountered at: %zu.\n", parser.current.line);
            return;
        }

        advance(&parser, &lexer);
    }
}
