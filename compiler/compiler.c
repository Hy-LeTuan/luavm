#include <compiler.h>

#include <lexer.h>
#include <object.h>
#include <objstring.h>
#include <objclosure.h>
#include <memory.h>
#include <reserved_var.h>

#include <string.h>
#include <stdio.h>

static void initCompiler(Compiler* compiler, Compiler* prev)
{
    compiler->enclosing = prev;
    compiler->function = NULL;

    compiler->currentScope = 0;
    compiler->currentLoopScope = 0;
    compiler->localCount = 0;
    compiler->breakCount = 0;

    ObjFunction* function = newFunction();
    compiler->function = function;
}

static void initParser(Parser* parser, const char* source, Table* strings, Compiler* compiler)
{
    initLexer(source, &parser->lexer);

    initCompiler(compiler, NULL);

    parser->compiler = compiler;
    parser->currentPrec = PREC_NONE;
    parser->hadError = false;
    parser->strings = strings;
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
    return &parser->compiler->function->chunk;
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

static size_t identifierConstant(Token* name, Parser* parser)
{
    Value name_obj = OBJ_VAL((Object*)copyString(name->start, name->length, parser->strings));

    size_t pos = addConstant(currentChunk(parser), name_obj);
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

static void emitReturn(Parser* parser)
{
    emitConstant(NIL_CONSTANT, parser);
    emitByte(OP_RETURN, parser);
}

static size_t getLoopStart(Parser* parser)
{
    return currentChunk(parser)->count;
}

static void beginScope(Parser* parser)
{
    parser->compiler->currentScope++;
}

static void endScope(Parser* parser)
{
    parser->compiler->currentScope--;

    while (parser->compiler->localCount > 0 &&
      parser->compiler->locals[parser->compiler->localCount - 1].scope >
        parser->compiler->currentScope)
    {
        parser->compiler->locals[parser->compiler->localCount - 1].isCaptured
          ? emitByte(OP_CLOSE_UPVALUE, parser)
          : emitByte(OP_POP, parser);
        parser->compiler->localCount--;
    }
}

static void endScopeUntil(int targetScope, Parser* parser)
{
    size_t i = parser->compiler->localCount;

    while (i > 0 && parser->compiler->locals[i - 1].scope > targetScope)
    {
        parser->compiler->locals[i - 1].isCaptured ? emitByte(OP_CLOSE_UPVALUE, parser)
                                                   : emitByte(OP_POP, parser);
        i--;
    }
}

static void beginLoop(Parser* parser)
{
    parser->compiler->currentLoopScope++;
    int* loopContext = &parser->compiler->loopContexts[parser->compiler->currentLoopScope];
    *loopContext = parser->compiler->currentScope;
}

static void endLoop(Parser* parser)
{
    parser->compiler->currentLoopScope--;

    while (parser->compiler->breakCount > 0 &&
      parser->compiler->breaks[parser->compiler->breakCount - 1].depth >
        parser->compiler->currentLoopScope)
    {
        Break* br = &parser->compiler->breaks[parser->compiler->breakCount - 1];
        patchJump(br->pos, parser);
        parser->compiler->breakCount--;
    }
}

static Token genReservedVarToken(ReservedVarType type)
{
    ReservedVar* reserved = &ReservedVarTable[type];
    Token token = {
        .type = TOKEN_IDENTIFIER,
        .length = reserved->length,
        .line = -1,
        .start = reserved->name,
    };

    return token;
}

/*
Resets parser to previous function and return function held by popped compiler.
*/
static ObjFunction* endCompiler(Parser* parser)
{
    emitReturn(parser);

    ObjFunction* function = parser->compiler->function;
    parser->compiler = parser->compiler->enclosing;

    return function;
}

// common declaration
static void parse(Precedence prevPrec, Parser* parser);
static void statements(Parser* parser);
static void functionStatement(Parser* parser, bool local, bool anonymous);
static void expression(Parser* parser);
Rule* getRule(TokenType op);

static int lookupLocal(Token* name, Compiler* compiler)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local* local = &compiler->locals[i];

        if (local->scope == -1 || local->scope > compiler->currentScope)
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
    parser->compiler->locals[localIndex].scope = parser->compiler->currentScope;
}

static int addUpvalue(uint8_t index, bool immediate, Compiler* compiler)
{
    int upvalueCount = compiler->upvalueCount;

    for (int i = 0; i < upvalueCount; i++)
    {
        Upvalue* upvalue = &compiler->upvalues[i];

        // upvalue already exists
        if (upvalue->index == index && upvalue->immediate == immediate)
        {
            return i;
        }
    }

    if (upvalueCount >= INT8_MAX)
    {
        return -1;
    }

    Upvalue* upvalue = &compiler->upvalues[upvalueCount];
    upvalue->index = index;
    upvalue->immediate = immediate;

    compiler->upvalueCount++;

    return upvalueCount;
}

static int lookupUpvalue(Token* name, Compiler* compiler)
{
    if (compiler->enclosing == NULL)
    {
        return -1;
    }

    int index = lookupLocal(name, compiler->enclosing);

    if (index != -1)
    {
        // add upvalue to the closure that actual needs the upvalue, not where it is found
        compiler->enclosing->locals[index].isCaptured = true;
        addUpvalue((uint8_t)index, true, compiler);
        return index;
    }

    index = lookupUpvalue(name, compiler->enclosing);
    if (index != -1)
    {
        addUpvalue((uint8_t)index, false, compiler);
        return index;
    }

    return -1;
}

static void namedVariable(Parser* parser)
{
    Token name = parser->prev;
    uint8_t var_constant = identifierConstant(&name, parser);

    uint8_t opGet;
    uint8_t opSet;
    int index = lookupLocal(&name, parser->compiler);

    if (index != -1)
    {
        opGet = OP_GET_LOCAL;
        opSet = OP_SET_LOCAL;
    }
    else if ((index = lookupUpvalue(&name, parser->compiler)) != -1)
    {
        opGet = OP_GET_UPVALUE;
        opSet = OP_SET_UPVALUE;
    }
    else
    {
        opGet = OP_GET_GLOBAL;
        opSet = OP_SET_GLOBAL;
        index = var_constant;
    }

    if (parser->currentPrec <= PREC_ASSIGNMENT && match(TOKEN_EQUAL, parser))
    {
        if (match(TOKEN_FUNCTION, parser))
        {
            functionStatement(parser, false, true);
        }
        else
        {
            // consume the expression
            expression(parser);
        }
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
    TokenType op = prev(parser);
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
    else if (op == TOKEN_DOT_DOT)
    {
        emitByte(OP_JOIN, parser);
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

static uint8_t argumentList(Parser* parser)
{
    uint8_t arity = 0;

    if (!check(TOKEN_RIGHT_PAREN, parser))
    {
        do
        {
            expression(parser);

            if (arity > 255)
            {
                error("Can't have more than 255 arguments.", parser);
            }
            arity++;
        } while (match(TOKEN_COMMA, parser));
    }

    consume(TOKEN_RIGHT_PAREN, "Error, missing ')' to close function call.", parser);

    return arity;
}

static void call(Parser* parser)
{
    uint8_t arity = argumentList(parser);
    emitBytes(OP_CALL, arity, parser);
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
    [TOKEN_LEFT_PAREN] = { grouping, call, PREC_CALL },
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
    [TOKEN_DOT_DOT] = { NULL, binary, PREC_JOIN },
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
    Local* local = &parser->compiler->locals[parser->compiler->localCount];
    local->start = name->start;
    local->length = name->length;
    local->scope = -1;
    local->isCaptured = false;

    parser->compiler->localCount++;
    return parser->compiler->localCount - 1;
}

static uint8_t defineLocalVar(Token* name, Parser* parser)
{
    uint8_t constantIndex = identifierConstant(name, parser);
    uint8_t localIndex = addLocal(name, parser);

    return localIndex;
}

/*
Generate a `ObjFunction` object that represents the function currently defined.

When called in anonymous mode (with `anonymous = true`), the function generates the object only and
let the assignment to a variable be handled by the assignment statement. Otherwise, it simulates the
assignemnt process by defining the variable (globally or locally with `local`) based on the provided
function name.

Parameters:
- `local`: The option to denote whether this function is defined locally or not
- `anonymous`: The option to denote whether this function is anonymous or not
*/
static void functionStatement(Parser* parser, bool local, bool anonymous)
{
    uint8_t var_constant = 0;
    if (!anonymous)
    {
        consume(TOKEN_IDENTIFIER,
          "Error, expect function name after 'function' and before arguments.", parser);

        Token name = parser->prev;

        if (local)
        {
            uint8_t local = defineLocalVar(&name, parser);
            markInitialized(local, parser);
        }
        else
        {
            var_constant = identifierConstant(&name, parser);
        }
    }

    consume(TOKEN_LEFT_PAREN, "Error, expect '(' after.", parser);

    Compiler compiler;
    initCompiler(&compiler, parser->compiler);

    beginScope(parser);

    /* start parsing current function */
    parser->compiler = &compiler;

    if (!check(TOKEN_RIGHT_PAREN, parser))
    {
        do
        {
            parser->compiler->function->arity++;
            if (parser->compiler->function->arity > 255)
            {
                error("Can't have more than 255 parameters.", parser);
            }

            consume(TOKEN_IDENTIFIER, "Error, expect parameter name.", parser);
            Token name = parser->prev;
            uint8_t constant = defineLocalVar(&name, parser);
            markInitialized(constant, parser);
        } while (match(TOKEN_COMMA, parser));
    }

    consume(TOKEN_RIGHT_PAREN, "Error, expect ')' after argument list.", parser);

    block("Error, expect 'end' after function definition.", parser);

    // endCompiler emits a return that would already reset the stack, no need for endScope
    ObjFunction* function = endCompiler(parser);
    function->upvalueCount = compiler.upvalueCount;

    /* finish parsing current function */

    ObjClosure* closure = newClosure(function);

    size_t constant = addConstant(currentChunk(parser), OBJ_VAL((Object*)function));

    emitBytes(OP_CLOSURE, constant, parser);

    // add upvalues
    for (int i = 0; i < function->upvalueCount; i++)
    {
        Upvalue* upvalue = &compiler.upvalues[i];

        emitByte(upvalue->immediate ? 1 : 0, parser);
        emitByte((uint8_t)upvalue->index, parser);
    }

    // set function as a global variable
    if (!anonymous && !local)
    {
        emitBytes(OP_SET_GLOBAL, var_constant, parser);
        emitByte(OP_POP, parser);
    }
}

static void localVarStatement(Parser* parser)
{
    consume(TOKEN_IDENTIFIER, "Error, an identifier is required after 'local' keyword.", parser);
    Token name = parser->prev;
    uint8_t localIndex = defineLocalVar(&name, parser);

    if (match(TOKEN_EQUAL, parser))
    {
        if (match(TOKEN_FUNCTION, parser))
        {
            functionStatement(parser, true, true);
        }
        else
        {
            expression(parser);
        }
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

static void numericalFor(Token* loop_var, Parser* parser)
{
    beginScope(parser);

    // initializer
    expression(parser);
    consume(
      TOKEN_COMMA, "Error, no comma to separate between iniitializer, limit and step.", parser);

    // limit
    expression(parser);

    // step
    if (match(TOKEN_COMMA, parser))
    {
        expression(parser);
    }
    else
    {
        emitConstant(NUM_VAL(1), parser);
    }

    // define custom variables by the compiler
    Token var = genReservedVarToken(RESERVED_VAR);
    size_t var_index = defineLocalVar(&var, parser);
    markInitialized(var_index, parser);

    Token limit = genReservedVarToken(RESERVED_LIMIT);
    size_t limit_index = defineLocalVar(&limit, parser);
    markInitialized(limit_index, parser);

    Token step = genReservedVarToken(RESERVED_STEP);
    size_t step_index = defineLocalVar(&step, parser);
    markInitialized(step_index, parser);

    size_t zero_constant = addConstant(currentChunk(parser), NUM_VAL(0));

    /* begin loop */
    beginLoop(parser);
    size_t loopStart = getLoopStart(parser);
    int outerLoopScope = parser->compiler->currentScope;

    /* start condition */

    // step > 0
    emitBytes(OP_GET_LOCAL, step_index, parser);
    emitBytes(OP_CONSTANT, (uint8_t)zero_constant, parser);
    emitByte(OP_GREATER, parser);

    // and
    size_t andJump1 = emitJump(OP_JUMP_IF_FALSE, parser);
    emitByte(OP_POP, parser);

    // var <= limit
    emitBytes(OP_GET_LOCAL, var_index, parser);
    emitBytes(OP_GET_LOCAL, limit_index, parser);
    emitBytes(OP_GREATER, OP_NOT, parser);

    // patch andJump
    patchJump(andJump1, parser);

    size_t orNextJump = emitJump(OP_JUMP_IF_FALSE, parser);
    size_t orEndJump = emitJump(OP_JUMP, parser);

    patchJump(orNextJump, parser);
    emitByte(OP_POP, parser);

    // step <= 0
    emitBytes(OP_GET_LOCAL, step_index, parser);
    emitBytes(OP_CONSTANT, (uint8_t)zero_constant, parser);
    emitBytes(OP_GREATER, OP_NOT, parser);

    // and
    size_t andJump2 = emitJump(OP_JUMP_IF_FALSE, parser);
    emitByte(OP_POP, parser);

    // var >= limit
    emitBytes(OP_GET_LOCAL, var_index, parser);
    emitBytes(OP_GET_LOCAL, limit_index, parser);
    emitBytes(OP_NOT, OP_LESS, parser);

    // patch andJump
    patchJump(andJump2, parser);

    // patch or jump
    patchJump(orEndJump, parser);

    /* end condition */

    /* start body */

    size_t endLoopJump = emitJump(OP_JUMP_IF_FALSE, parser);
    emitByte(OP_POP, parser);

    consume(TOKEN_DO, "Error, expect token 'do' after for initializations.", parser);
    beginScope(parser);

    // load loop_var
    emitBytes(OP_GET_LOCAL, var_index, parser);
    size_t loop_var_index = defineLocalVar(loop_var, parser);
    emitBytes(OP_SET_LOCAL, loop_var_index, parser);
    markInitialized(loop_var_index, parser);

    block("Error, missing token 'end' to close for statement.", parser);

    // increment var via assignment expression
    emitBytes(OP_GET_LOCAL, var_index, parser);
    emitBytes(OP_GET_LOCAL, step_index, parser);
    emitByte(OP_ADD, parser);
    emitBytes(OP_SET_LOCAL, var_index, parser);
    emitByte(OP_POP, parser);

    endScope(parser);

    /* end body */

    emitLoop(loopStart, parser);

    patchJump(endLoopJump, parser);
    emitByte(OP_POP, parser);
    endLoop(parser);

    /* end loop*/

    endScope(parser);
}

static void forStatement(Parser* parser)
{
    consume(TOKEN_IDENTIFIER, "Error, identifier needs to follow 'for' statement.", parser);
    Token loop_var = parser->prev;

    if (match(TOKEN_EQUAL, parser))
    {
        numericalFor(&loop_var, parser);
    }
    else if (match(TOKEN_IN, parser))
    {
        // TODO: implement generic for
    }
    else
    {
        error("Error, unknown keyword found after 'for'.", parser);
    }
}

static void breakStatement(Parser* parser)
{
    if (parser->compiler->currentLoopScope == 0)
    {
        errorAt(parser->prev, "Error, a break statement cannot appear outside of a loop.", parser);
    }

    // clean up until we meet the outer scope of the loop
    int loopScope = parser->compiler->loopContexts[parser->compiler->currentLoopScope];

    endScopeUntil(loopScope, parser);

    size_t breakJump = emitJump(OP_JUMP, parser);
    Break* br = &parser->compiler->breaks[parser->compiler->breakCount];
    br->pos = breakJump;
    br->depth = parser->compiler->currentLoopScope;
    parser->compiler->breakCount++;
}

static void returnStatement(Parser* parser)
{
    if (getRule(peek(parser))->prefix != NULL)
    {
        expression(parser);
        emitByte(OP_RETURN, parser);
    }
    else if (match(TOKEN_FUNCTION, parser))
    {
        functionStatement(parser, true, true);
        emitByte(OP_RETURN, parser);
    }
    else
    {
        emitConstant(NIL_CONSTANT, parser);
        emitByte(OP_RETURN, parser);
    }
}

static void statements(Parser* parser)
{
    bool hasExpression = false;
    if (match(TOKEN_LOCAL, parser))
    {
        if (match(TOKEN_FUNCTION, parser))
        {
            functionStatement(parser, true, false);
        }
        else
        {
            localVarStatement(parser);
        }

        hasExpression = true;
    }
    else if (match(TOKEN_FUNCTION, parser))
    {
        functionStatement(parser, false, false);
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
    else if (match(TOKEN_FOR, parser))
    {
        forStatement(parser);
        hasExpression = true;
    }
    else if (match(TOKEN_RETURN, parser))
    {
        returnStatement(parser);
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

ObjFunction* compile(const char* source, Table* strings)
{
    Parser parser;
    Compiler compiler;

    initParser(&parser, source, strings, &compiler);

    // first setup
    advance(&parser);

    // continously parse the list of statements into a chunk
    while (!match(TOKEN_EOF, &parser))
    {
        statements(&parser);
    }

    if (parser.hadError)
    {
        fprintf(stderr, "Compiler error, exiting now.\n");
        exit(EXIT_FAILURE);
    }

    ObjFunction* function = endCompiler(&parser);
    return function;
}
