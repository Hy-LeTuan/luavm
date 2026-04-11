#include <compiler.h>

#include <lexer.h>
#include <object.h>
#include <memory.h>

#include <stdio.h>

static void initParser(Parser* parser, const char* source, Chunk* chunk)
{
    initLexer(source, &parser->lexer);
    parser->hadError = false;
    parser->chunk = chunk;
}

static void errorAt(Token at, const char* message, Parser* parser)
{
    parser->hadError = true;
    fprintf(stderr, "Compile Error on line [%zu], at %.*s.\n", at.line, (int)at.length, at.start);
    fprintf(stderr, "Error reads: %s\n", message);
}

static void error(const char* message, Parser* parser)
{
    errorAt(parser->current, message, parser);
}

static void advance(Parser* parser)
{
    parser->prev = parser->current;

    for (;;)
    {
        Token token = lex(&parser->lexer);

        if (token.type == TOKEN_ERROR)
        {
            parser->hadError = true;
            errorAt(token, "Lexer Error, cannot parse the current word.", parser);
            return;
        }

        parser->current = token;
        break;
    }
}

static Chunk* currentChunk(Parser* parser)
{
    return parser->chunk;
}

static TokenType peek(Parser* parser)
{
    return parser->current.type;
}

static TokenType prev(Parser* parser)
{
    return parser->prev.type;
}

static void consume(TokenType token, const char* message, Parser* parser)
{
    if (peek(parser) != token)
    {
        fprintf(stdout, "%s\n", message);
        exit(EXIT_FAILURE);
    }

    advance(parser);
}

static void emitByte(uint8_t op, Parser* parser)
{
    writeChunk(parser->chunk, op, parser->prev.line);
}

static void emitBytes(uint8_t op1, uint8_t op2, Parser* parser)
{
    emitByte(op1, parser);
    emitByte(op2, parser);
}

static void emitConstant(Value value, Parser* parser)
{
    size_t pos = addConstant(parser->chunk, value);
    emitBytes(OP_CONSTANT, pos, parser);
}

// common declaration
static void parse(Precedence prevPrec, Parser* parser);
Rule* getRule(TokenType op);

static void unary(Parser* parser)
{
    TokenType op = prev(parser);

    // all negate operators are right associative
    parse(PREC_UNARY, parser);

    switch (op)
    {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE, parser);
            break;
        default:
            fprintf(stderr, "Invalid prefix operator.\n");
            exit(EXIT_FAILURE);
    }
}

static void grouping(Parser* parser)
{
    // parse expression
    parse(PREC_ASSIGNMENT, parser);
    consume(TOKEN_RIGHT_PAREN, "Error, expect ')' after an opened '('.", parser);
}

static void binary(Parser* parser)
{
    TokenType op = parser->prev.type;
    Rule* rule = getRule(op);

    // right associative
    if (op == TOKEN_CARET)
    {
        parse(rule->prec, parser);
    }
    // left associative
    else
    {
        parse(rule->prec + 1, parser);
    }

    if (op == TOKEN_PLUS)
    {
        emitByte(OP_ADD, parser);
    }
    else if (op == TOKEN_MINUS)
    {
        emitByte(OP_MINUS, parser);
    }
    else if (op == TOKEN_STAR)
    {
        emitByte(OP_MUL, parser);
    }
    else if (op == TOKEN_SLASH)
    {
        emitByte(OP_DIV, parser);
    }
    else if (op == TOKEN_CARET)
    {
        emitByte(OP_EXPONENT, parser);
    }
    else if (op == TOKEN_PERCENT)
    {
        emitByte(OP_MODULO, parser);
    }
    else
    {
        fprintf(stderr, "Error, cannot parse binary operator.\n");
        exit(EXIT_FAILURE);
    }
}

static void number(Parser* parser)
{
    double num = strtod(parser->prev.start, NULL);
    Value value = NUM_VAL(num);
    emitConstant(value, parser);
}

static void str(Parser* parser)
{
    const char* text = parser->prev.start + 1;
    size_t length = parser->prev.length - 2;

    ObjString* string = copyString(text, length);
    emitConstant(OBJ_VAL((Object*)string), parser);
}

// pratt parsing table
Rule rules[] = { [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
    [TOKEN_MINUS] = { unary, binary, PREC_TERM },
    [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
    [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
    [TOKEN_PERCENT] = { NULL, binary, PREC_FACTOR },
    [TOKEN_CARET] = { NULL, binary, PREC_EXPONENT },
    [TOKEN_HASH] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS] = { NULL, NULL, PREC_NONE },
    [TOKEN_GREATER] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_PAREN] = { grouping, NULL, PREC_NONE },
    [TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_SQUARE] = { NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_SQUARE] = { NULL, NULL, PREC_NONE },
    [TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
    [TOKEN_COLON] = { NULL, NULL, PREC_NONE },
    [TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_TILDE_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_GREATER_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_THREE_DOTS] = { NULL, NULL, PREC_NONE },
    [TOKEN_AND] = { NULL, NULL, PREC_NONE },
    [TOKEN_BREAK] = { NULL, NULL, PREC_NONE },
    [TOKEN_DO] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSEIF] = { NULL, NULL, PREC_NONE },
    [TOKEN_END] = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_FOR] = { NULL, NULL, PREC_NONE },
    [TOKEN_FUNCTION] = { NULL, NULL, PREC_NONE },
    [TOKEN_IF] = { NULL, NULL, PREC_NONE },
    [TOKEN_IN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LOCAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_NIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_NOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_OR] = { NULL, NULL, PREC_NONE },
    [TOKEN_REPEAT] = { NULL, NULL, PREC_NONE },
    [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
    [TOKEN_THEN] = { NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] = { NULL, NULL, PREC_NONE },
    [TOKEN_UNTIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_IDENTIFIER] = { NULL, NULL, PREC_NONE },
    [TOKEN_STRING] = { str, NULL, PREC_NONE },
    [TOKEN_NUMBER] = { number, NULL, PREC_NONE },
    [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] = { NULL, NULL, PREC_NONE } };

Rule* getRule(TokenType op)
{
    return &rules[op];
}

static void parse(Precedence prevPrec, Parser* parser)
{
    advance(parser);

    TokenType prefixOp = prev(parser);
    ParseFn prefixFn = getRule(prefixOp)->prefix;

    if (prefixFn == NULL)
    {
        fprintf(stderr, "Error, missing prefix rule.\n");
        exit(EXIT_FAILURE);
    }

    // left verse
    prefixFn(parser);

    Rule* infixRule;
    while ((infixRule = getRule(peek(parser)))->prec >= prevPrec)
    {
        // move to the next prefix starting point
        advance(parser);
        ParseFn infixFn = infixRule->infix;

        if (infixFn == NULL)
        {
            fprintf(stderr, "Error, missing infix rule.\n");
        }

        infixFn(parser);
    }
}

static void expression(Parser* parser)
{
    parse(PREC_ASSIGNMENT, parser);
}

void compile(const char* source, Chunk* chunk)
{
    Parser parser;
    initParser(&parser, source, chunk);

    // first setup
    advance(&parser);

    expression(&parser);
    emitByte(OP_RETURN, &parser);

    if (parser.hadError)
    {
        fprintf(stderr, "Compiler error, exiting now.\n");
        exit(EXIT_FAILURE);
    }
}
