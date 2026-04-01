#ifndef compiler_compiler_h
#define compiler_compiler_h

#include <token.h>

void compile(const char* source);

typedef struct
{
    Token prev;
    Token current;
    bool hadError;
} Parser;

#endif
