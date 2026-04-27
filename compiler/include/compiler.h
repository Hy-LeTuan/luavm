#ifndef compiler_compiler_h
#define compiler_compiler_h

#include <lexer.h>
#include <token.h>
#include <chunk.h>
#include <table.h>
#include <locals.h>

void compile(const char* source, Chunk* chunk, Table* strings, Table* globals);

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_RELATIONAL,
    PREC_JOIN,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_EXPONENT
} Precedence;

typedef struct
{
    int depth;
    size_t pos;
} Break;

typedef struct
{
    Lexer lexer;
    Token prev;
    Token current;
    Precedence currentPrec;
    bool hadError;
    Chunk* chunk;
    Table* strings;
    Table* globals;
    int currentScope;
    int currentLoopScope;
    size_t localCount;
    size_t breakCount;
    int loopContexts[UINT16_MAX + 1];
    Local locals[UINT8_MAX + 1];
    Break breaks[UINT16_MAX + 1];
} Parser;

typedef void (*ParseFn)(Parser*);
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    // precedence of the infix operator
    Precedence prec;
} Rule;

#endif
