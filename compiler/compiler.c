#include <compiler.h>

#include <lexer.h>
#include <object.h>
#include <objstring.h>
#include <memory.h>

#include <string.h>
#include <stdio.h>

static void initParser(
  Parser* parser, const char* source, Chunk* chunk, Table* strings, Table* globals)
{
    initLexer(source, &parser->lexer);
    parser->currentScope = 0;
    parser->currentPrec = PREC_NONE;
    parser->hadError = false;
    parser->chunk = chunk;
    parser->strings = strings;
    parser->globals = globals;
    parser->localCount = 0;
    parser->currentLoopScope = 0;
    parser->breakCount = 0;
}

static bool isAtEnd(Parser* parser)
{
    return parser->current.type == TOKEN_EOF;
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

static bool check(TokenType type, Parser* parser)
{
    return parser->current.type == type;
}

static bool match(TokenType type, Parser* parser)
{
    if (!check(type, parser))
    {
        return false;
    }

    advance(parser);
    return true;
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
    writeChunk(currentChunk(parser), op, parser->prev.line);
}

static void emitBytes(uint8_t op1, uint8_t op2, Parser* parser)
{
    emitByte(op1, parser);
    emitByte(op2, parser);
}

static size_t emitJump(OPCode jumpCode, Parser* parser)
{
    emitByte(jumpCode, parser);
    emitByte(0, parser);
    emitByte(0, parser);

    return currentChunk(parser)->count - 2;
}

/*
 * Patch the operands of a jump statement to jump to the current bytecode
 * */
static void patchJump(size_t offset, Parser* parser)
{
    int jump = currentChunk(parser)->count - offset - 2;

    if (jump > UINT16_MAX)
    {
        error("Error, too much code to jump over.", parser);
    }

    currentChunk(parser)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(parser)->code[offset + 1] = jump & 0xff;
}

static void emitConstant(Value value, Parser* parser)
{
    size_t pos = addConstant(currentChunk(parser), value);
    emitBytes(OP_CONSTANT, pos, parser);
}

static size_t identifierConstant(Parser* parser)
{
    Value name =
      OBJ_VAL((Object*)copyString(parser->prev.start, parser->prev.length, parser->globals));

    size_t pos = addConstant(currentChunk(parser), name);
    return pos;
}

static void emitLoop(size_t start, Parser* parser)
{
    emitByte(OP_LOOP, parser);

    int jump = currentChunk(parser)->count - start + 2;

    if (jump > UINT16_MAX)
    {
        error("Error, loop body too large.", parser);
    }

    emitByte((jump >> 8) & 0xff, parser);
    emitByte(jump & 0xff, parser);
}

static size_t getLoopStart(Parser* parser)
{
    return currentChunk(parser)->count;
}

static void beginScope(Parser* parser)
{
    parser->currentScope++;
}

static void endScope(Parser* parser)
{
    parser->currentScope--;

    while (
      parser->localCount > 0 && parser->locals[parser->localCount - 1].scope > parser->currentScope)
    {
        emitByte(OP_POP, parser);
        parser->localCount--;
    }
}

static void beginLoop(Parser* parser)
{
    parser->currentLoopScope++;
}

static void endLoop(Parser* parser)
{
    parser->currentLoopScope--;

    while (
      parser->breakCount > 0 && parser->breaks[parser->breakCount - 1].depth > parser->currentScope)
    {
        Break* br = &parser->breaks[parser->breakCount - 1];
        patchJump(br->pos, parser);
        parser->breakCount--;
    }
}

// common declaration
static void parse(Precedence prevPrec, Parser* parser);
static void statements(Parser* parser);
static void expression(Parser* parser);
Rule* getRule(TokenType op);

static int lookupLocal(Token* name, Parser* parser)
{
    for (int i = parser->localCount - 1; i >= 0; i--)
    {
        Local* local = &parser->locals[i];

        if (local->scope == -1 || local->scope > parser->currentScope)
        {
            continue;
        }

        if (local->length == name->length && memcmp(local->start, name->start, name->length) == 0)
        {
            return i;
        }
    }

    return -1;
}

static void markInitialized(uint8_t localIndex, Parser* parser)
{
    parser->locals[localIndex].scope = parser->currentScope;
}

static void namedVariable(Parser* parser)
{
    Token* name = &parser->prev;
    uint8_t var_constant = identifierConstant(parser);

    uint8_t opGet;
    uint8_t opSet;
    int index = lookupLocal(name, parser);

    if (index != -1)
    {
        opGet = OP_GET_LOCAL;
        opSet = OP_SET_LOCAL;
    }
    else
    {
        opGet = OP_GET_GLOBAL;
        opSet = OP_SET_GLOBAL;
        index = var_constant;
    }

    if (parser->currentPrec <= PREC_ASSIGNMENT && match(TOKEN_EQUAL, parser))
    {
        // consume the expression
        expression(parser);
        emitBytes(opSet, (uint8_t)index, parser);
    }
    else
    {
        emitBytes(opGet, (uint8_t)index, parser);
    }
}

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
        case TOKEN_NOT:
            emitByte(OP_NOT, parser);
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

static void boolean(Parser* parser)
{
    if (prev(parser) == TOKEN_TRUE)
    {
        emitConstant(BOOL_VAL(true), parser);
    }
    else
    {
        emitConstant(BOOL_VAL(false), parser);
    }
}

static void str(Parser* parser)
{
    const char* text = parser->prev.start + 1;
    size_t length = parser->prev.length - 2;

    ObjString* string = copyString(text, length, parser->strings);
    emitConstant(OBJ_VAL((Object*)string), parser);
}

static void block(const char* message, Parser* parser)
{
    while (peek(parser) != TOKEN_END && !isAtEnd(parser))
    {
        statements(parser);
    }

    consume(TOKEN_END, message, parser);
}

static void relational(Parser* parser)
{
    TokenType op = prev(parser);
    parse(PREC_RELATIONAL + 1, parser);

    if (op == TOKEN_LESS)
    {
        emitByte(OP_LESS, parser);
    }
    else if (op == TOKEN_LESS_EQUAL)
    {
        emitByte(OP_GREATER, parser);
        emitByte(OP_NOT, parser);
    }
    else if (op == TOKEN_GREATER)
    {
        emitByte(OP_GREATER, parser);
    }
    else if (op == TOKEN_GREATER_EQUAL)
    {
        emitByte(OP_LESS, parser);
        emitByte(OP_NOT, parser);
    }
    else if (op == TOKEN_EQUAL_EQUAL)
    {
        emitByte(OP_EQUAL, parser);
    }
    else
    {
        emitByte(OP_EQUAL, parser);
        emitByte(OP_NOT, parser);
    }
}

/*
 * `or` operator leaves the first truthful value evaluated on the stack
 * */
static void logical_or(Parser* parser)
{
    size_t nextBranchJump = emitJump(OP_JUMP_IF_FALSE, parser);
    size_t endJump = emitJump(OP_JUMP, parser);

    patchJump(nextBranchJump, parser);
    emitByte(OP_POP, parser);

    parse(PREC_OR, parser);

    patchJump(endJump, parser);
}

/*
 * `and` operator leaves the last value evaluated on the stack
 * */
static void logical_and(Parser* parser)
{
    size_t endJump = emitJump(OP_JUMP_IF_FALSE, parser);

    emitByte(OP_POP, parser);
    parse(PREC_AND, parser);

    patchJump(endJump, parser);
}

// pratt parsing table
Rule rules[] = { [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
    [TOKEN_MINUS] = { unary, binary, PREC_TERM },
    [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
    [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
    [TOKEN_PERCENT] = { NULL, binary, PREC_FACTOR },
    [TOKEN_CARET] = { NULL, binary, PREC_EXPONENT },
    [TOKEN_HASH] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_GREATER] = { NULL, relational, PREC_RELATIONAL },
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
    [TOKEN_EQUAL_EQUAL] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_TILDE_EQUAL] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_LESS_EQUAL] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_GREATER_EQUAL] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_DOT_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_THREE_DOTS] = { NULL, NULL, PREC_NONE },
    [TOKEN_AND] = { NULL, logical_and, PREC_AND },
    [TOKEN_BREAK] = { NULL, NULL, PREC_NONE },
    [TOKEN_DO] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSEIF] = { NULL, NULL, PREC_NONE },
    [TOKEN_END] = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] = { boolean, NULL, PREC_NONE },
    [TOKEN_FOR] = { NULL, NULL, PREC_NONE },
    [TOKEN_FUNCTION] = { NULL, NULL, PREC_NONE },
    [TOKEN_IF] = { NULL, NULL, PREC_NONE },
    [TOKEN_IN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LOCAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_NIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_NOT] = { unary, NULL, PREC_NONE },
    [TOKEN_OR] = { NULL, logical_or, PREC_OR },
    [TOKEN_REPEAT] = { NULL, NULL, PREC_NONE },
    [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
    [TOKEN_THEN] = { NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] = { boolean, NULL, PREC_NONE },
    [TOKEN_UNTIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_IDENTIFIER] = { namedVariable, NULL, PREC_NONE },
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
    parser->currentPrec = prevPrec;

    TokenType prefixOp = prev(parser);
    ParseFn prefixFn = getRule(prefixOp)->prefix;

    if (prefixFn == NULL)
    {
        errorAt(parser->prev, "Error, missing prefix rule.", parser);
        exit(EXIT_FAILURE);
    }

    // left verse
    prefixFn(parser);

    Rule* infixRule;
    while ((infixRule = getRule(peek(parser)))->prec >= prevPrec)
    {
        // move to the next prefix starting point
        advance(parser);
        parser->currentPrec = infixRule->prec;

        ParseFn infixFn = infixRule->infix;

        if (infixFn == NULL)
        {
            fprintf(stderr, "Error, missing infix rule.\n");
        }

        infixFn(parser);
    }

    parser->currentPrec = prevPrec;
}

static void expression(Parser* parser)
{
    parse(PREC_ASSIGNMENT, parser);
}

static void expressionStatement(Parser* parser)
{
    expression(parser);
    emitByte(OP_POP, parser);
}

static uint8_t addLocal(Token* name, Parser* parser)
{
    Local* local = &parser->locals[parser->localCount];
    local->start = name->start;
    local->length = name->length;
    local->scope = -1;

    parser->localCount++;
    return parser->localCount - 1;
}

static void defineVariable(Parser* parser)
{
    consume(TOKEN_IDENTIFIER, "Error, an identifier is required after 'local' keyword.", parser);
    Token* name = &parser->prev;
    uint8_t constantIndex = identifierConstant(parser);

    uint8_t localIndex = addLocal(name, parser);

    if (match(TOKEN_EQUAL, parser))
    {
        expression(parser);
    }
    else
    {
        emitConstant(NIL_CONSTANT, parser);
    }

    markInitialized(localIndex, parser);
}

static void ifStatement(Parser* parser)
{
    // start of a single if branch
    expression(parser);
    consume(TOKEN_THEN, "Error, expect 'then' to follow after condition expression.", parser);

    size_t nextBranchJump = emitJump(OP_JUMP_IF_FALSE, parser);
    emitByte(OP_POP, parser);

    // body statements
    beginScope(parser);
    while (!isAtEnd(parser) && peek(parser) != TOKEN_ELSE && peek(parser) != TOKEN_ELSEIF &&
      peek(parser) != TOKEN_END)
    {
        statements(parser);
    }
    endScope(parser);
    size_t endJump = emitJump(OP_JUMP, parser);

    patchJump(nextBranchJump, parser);
    emitByte(OP_POP, parser);

    // possible parralel branches
    if (match(TOKEN_ELSEIF, parser))
    {
        ifStatement(parser);
    }
    else if (match(TOKEN_ELSE, parser))
    {
        beginScope(parser);
        block("Error, missing token 'end' to close if statement.", parser);
        endScope(parser);
    }
    else
    {
        consume(TOKEN_END, "Error, missing token 'end' to close if statement.", parser);
    }

    // jump to end of if-statement
    patchJump(endJump, parser);
}

static void whileStatement(Parser* parser)
{
    beginLoop(parser);
    size_t loopStart = getLoopStart(parser);

    expression(parser);
    size_t endJump = emitJump(OP_JUMP_IF_FALSE, parser);
    emitByte(OP_POP, parser);

    consume(TOKEN_DO, "Error, expect token 'do' after while statement.", parser);
    beginScope(parser);
    block("Error, missing token 'end' to close while statement", parser);
    endScope(parser);

    emitLoop(loopStart, parser);

    patchJump(endJump, parser);
    emitByte(OP_POP, parser);
    endLoop(parser);
}

static void repeatStatement(Parser* parser)
{
    beginLoop(parser);
    size_t loopStart = getLoopStart(parser);

    beginScope(parser);
    while (!isAtEnd(parser) && peek(parser) != TOKEN_UNTIL)
    {
        statements(parser);
    }
    consume(TOKEN_UNTIL, "Error, expect until before condition of repeat statement.", parser);
    expression(parser);
    // flip condition to loop while exit condition is false
    emitByte(OP_NOT, parser);
    endScope(parser);

    size_t endJump = emitJump(OP_JUMP_IF_FALSE, parser);

    emitByte(OP_POP, parser);
    emitLoop(loopStart, parser);

    patchJump(endJump, parser);
    emitByte(OP_POP, parser);
    endLoop(parser);
}

static void breakStatement(Parser* parser)
{
    if (parser->currentLoopScope == 0)
    {
        errorAt(parser->prev, "Error, a break statement cannot appear outside of a loop.", parser);
    }

    size_t breakJump = emitJump(OP_JUMP, parser);
    Break* br = &parser->breaks[parser->breakCount];
    br->pos = breakJump;
    br->depth = parser->currentLoopScope;
    parser->breakCount++;
}

static void statements(Parser* parser)
{
    bool hasExpression = false;
    if (match(TOKEN_LOCAL, parser))
    {
        // local function
        if (match(TOKEN_FUNCTION, parser))
        {
        }
        // local variable
        else
        {
            defineVariable(parser);
        }

        hasExpression = true;
    }
    else if (match(TOKEN_FUNCTION, parser))
    {
        hasExpression = true;
    }
    else if (match(TOKEN_DO, parser))
    {
        beginScope(parser);
        block("Error, missing token 'end' to close block.", parser);
        endScope(parser);
        hasExpression = true;
    }
    else if (match(TOKEN_IF, parser))
    {
        ifStatement(parser);
        hasExpression = true;
    }
    else if (match(TOKEN_WHILE, parser))
    {
        whileStatement(parser);
        hasExpression = true;
    }
    else if (match(TOKEN_REPEAT, parser))
    {
        repeatStatement(parser);
        hasExpression = true;
    }
    else if (match(TOKEN_BREAK, parser))
    {
        breakStatement(parser);
        hasExpression = true;
    }
    else
    {
        expressionStatement(parser);
        hasExpression = true;
    }

    // consume an optinal semicolon
    if (peek(parser) == TOKEN_SEMICOLON)
    {
        advance(parser);
    }

    if (!hasExpression)
    {
        error("Erorr, cannot have an empty statement.", parser);
    }
}

void compile(const char* source, Chunk* chunk, Table* strings, Table* globals)
{
    Parser parser;
    initParser(&parser, source, chunk, strings, globals);

    // first setup
    advance(&parser);

    // continously parse the list of statements into a chunk
    while (!match(TOKEN_EOF, &parser))
    {
        statements(&parser);
    }

    emitByte(OP_RETURN, &parser);

    if (parser.hadError)
    {
        fprintf(stderr, "Compiler error, exiting now.\n");
        exit(EXIT_FAILURE);
    }
}
