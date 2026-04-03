#ifndef lexer_lexer_h
#define lexer_lexer_h

#include <token.h>

typedef struct
{
    const char* start;
    const char* current;

    size_t line;
} Lexer;

void initLexer(const char* source, Lexer* lexer);
Token lex(Lexer* lexer);

#ifdef DEBUG_PRINT_TOKEN
const char* convertToken(Token* token);
#endif

#endif
