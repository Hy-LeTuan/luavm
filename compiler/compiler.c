#include <compiler.h>
#include <lexer.h>

#include <stdio.h>

static void initParser(Parser* parser, const char* source, Chunk* chunk)
{
    initLexer(source, &parser->lexer);
    parser->hadError = false;
    parser->chunk = chunk;
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
            fprintf(stderr, "Error at line: %zu\n", token.line);

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
    TokenType op = parser->prev.type;

    // all negate operators are right associative
    parse(PREC_UNARY, parser);

    switch (op)
    {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE, parser);
        default:
            fprintf(stderr, "Invalid prefix operator.\n");
            exit(1);
    }
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
    else
    {
        emitByte(OP_EXPONENT, parser);
    }
}

static void number(Parser* parser)
{
    double num = strtod(parser->prev.start, NULL);
    Value value = NUM_VAL(num);
    emitConstant(value, parser);
}

// pratt parsing table
Rule rules[] = { [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
    [TOKEN_MINUS] = { unary, binary, PREC_TERM },
    [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
    [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
    [TOKEN_PERCENT] = { NULL, NULL, PREC_NONE },
    [TOKEN_CARET] = { NULL, binary, PREC_EXPONENT },
    [TOKEN_HASH] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS] = { NULL, NULL, PREC_NONE },
    [TOKEN_GREATER] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_PAREN] = { NULL, NULL, PREC_NONE },
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
    [TOKEN_STRING] = { NULL, NULL, PREC_NONE },
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

    TokenType prefixOp = parser->prev.type;
    ParseFn prefixFn = getRule(prefixOp)->prefix;

    if (prefixFn == NULL)
    {
        fprintf(stderr, "Error, missing prefix rule.\n");
        exit(EXIT_FAILURE);
    }

    // left verse
    prefixFn(parser);

    Rule* infixRule;
    while ((infixRule = getRule(parser->current.type))->prec >= prevPrec)
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

    advance(&parser);

    expression(&parser);
    emitByte(OP_RETURN, &parser);

    if (parser.hadError)
    {
        fprintf(stderr, "[TOKEN_ERROR] encountered at: %zu.\n", parser.current.line);
        return;
    }
}
