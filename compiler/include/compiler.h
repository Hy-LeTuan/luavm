#ifndef compiler_compiler_h
#define compiler_compiler_h

#include <lexer.h>
#include <token.h>
#include <chunk.h>
#include <object.h>
#include <table.h>
#include <locals.h>
#include <upvalues.h>

#define MULTRET 0
#define ZERORET 1
#define SINGLERET 2

#define HAS_MULTRET(k) (k == EXP_CALL || k == EXP_VARARG)
#define HAS_ASSIGN(k) (k == EXP_LOCAL || k == EXP_UPVAL || k == EXP_INDEX || k == EXP_GLOBAL)
#define ALLOW_VARARG(f) (f == TYPE_VARARG || f == TYPE_VARARG_NO_ARG)

ObjFunction* compile(const char* source, Table* strings);

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
    PREC_EXPONENT,
    PREC_CALL
} Precedence;

typedef struct
{
    int depth;
    size_t pos;
} Break;

typedef struct Compiler
{
    struct Compiler* enclosing;
    ObjFunction* function;

    // semantic analysis
    int currentScope;
    int currentLoopScope;
    size_t localCount;
    size_t breakCount;
    size_t upvalueCount;
    size_t loopContexts[UINT8_MAX + 1];
    Local locals[UINT8_MAX + 1];
    Upvalue upvalues[UINT8_MAX + 1];
    Break breaks[UINT8_MAX + 1];
} Compiler;

typedef struct
{
    Lexer lexer;
    Token prev;
    Token current;
    Precedence currentPrec;
    bool hadError;
    Table* strings;
    Compiler* compiler;
} Parser;

typedef enum
{
    EXP_VOID, /* no value */
    EXP_NIL,
    EXP_TRUE,
    EXP_FALSE,
    EXP_NUM,
    EXP_STR,
    EXP_LOCAL,
    EXP_UPVAL,
    EXP_INDEX, /* index into a table or array */
    EXP_GLOBAL,
    EXP_JMP,
    EXP_CALL,
    EXP_VARARG
} ExpKind;

typedef struct
{
    ExpKind kind;
    // the patch position if the op code requires it
    size_t patch;
} ExpDesc;

typedef void (*ParseFn)(ExpDesc* e, Parser*);
typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    // precedence of the infix operator
    Precedence prec;
} Rule;

typedef struct
{
    ExpDesc e;
    int fieldCount;
    uint8_t index;
} LhsAssign;

#endif
