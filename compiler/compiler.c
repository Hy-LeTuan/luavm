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
    compiler->upvalueCount = 0;

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

static void initExpDesc(ExpDesc* e)
{
    e->kind = EXP_VOID;
    e->patch = -1;
}

static bool isAtEnd(Parser* p)
{
    return p->current.type == TOKEN_EOF;
}

static void errorAt(Token at, const char* message, Parser* p)
{
    p->hadError = true;
    fprintf(stderr, "Compile Error on line [%zu], at %.*s.\n", at.line, (int)at.length, at.start);
    fprintf(stderr, "Error reads: %s\n", message);
}

static void error(const char* message, Parser* p)
{
    errorAt(p->current, message, p);
}

static void advance(Parser* p)
{
    p->prev = p->current;

    for (;;)
    {
        Token token = lex(&p->lexer);

        if (token.type == TOKEN_ERROR)
        {
            p->hadError = true;
            errorAt(token, "Lexer Error, cannot parse the current word.", p);
            return;
        }

        p->current = token;
        break;
    }
}

static bool check(TokenType type, Parser* p)
{
    return p->current.type == type;
}

static bool match(TokenType type, Parser* p)
{
    if (!check(type, p))
    {
        return false;
    }

    advance(p);
    return true;
}

static bool blockFollow(TokenType type)
{
    switch (type)
    {
        case TOKEN_ELSE:
        case TOKEN_ELSEIF:
        case TOKEN_END:
        case TOKEN_UNTIL:
        case TOKEN_EOF:
            return true;
        default:
            return false;
    }
}

static Chunk* currentChunk(Parser* p)
{
    return &p->compiler->function->chunk;
}

static TokenType peek(Parser* p)
{
    return p->current.type;
}

static TokenType prev(Parser* p)
{
    return p->prev.type;
}

static TokenType current(Parser* p)
{
    return p->current.type;
}

static void consume(TokenType token, const char* message, Parser* p)
{
    if (peek(p) != token)
    {
        fprintf(stdout, "%s\n", message);
        exit(EXIT_FAILURE);
    }

    advance(p);
}

static void emitByte(uint8_t op, Parser* p)
{
    writeChunk(currentChunk(p), op, p->prev.line);
}

static void emitBytes(uint8_t op1, uint8_t op2, Parser* p)
{
    emitByte(op1, p);
    emitByte(op2, p);
}

static void emitBytesABC(uint8_t A, uint8_t B, uint8_t C, Parser* p)
{
    emitByte(A, p);
    emitByte(B, p);
    emitByte(C, p);
}

static size_t emitJump(OPCode jumpCode, Parser* p)
{
    emitByte(jumpCode, p);
    emitByte((0 >> 8) & 0xff, p);
    emitByte(0 & 0xff, p);

    return currentChunk(p)->count - 2;
}

/*
 * Patch the operands of a jump statement to jump to the current bytecode
 * */
static void patchJump(size_t offset, Parser* p)
{
    int jump = currentChunk(p)->count - offset - 2;

    if (jump > UINT16_MAX)
    {
        error("Error, too much code to jump over.", p);
    }

    currentChunk(p)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(p)->code[offset + 1] = jump & 0xff;
}

static void patchSingleByte(size_t offset, uint8_t value, Parser* p)
{
    currentChunk(p)->code[offset] = value;
}

static void emitConstant(Value value, Parser* p)
{
    size_t pos = addConstant(currentChunk(p), value);
    emitBytes(OP_CONSTANT, pos, p);
}

static size_t identifierConstant(Token* name, Parser* p)
{
    Value name_obj = OBJ_VAL(copyString(name->start, name->length, p->strings));

    size_t pos = addConstant(currentChunk(p), name_obj);
    return pos;
}

static void emitBackJump(OPCode jumpCode, size_t start, Parser* p)
{
    emitByte(jumpCode, p);

    int jump = currentChunk(p)->count - start + 2;

    if (jump > UINT16_MAX)
    {
        error("Error, back jump too large.", p);
    }

    emitByte((jump >> 8) & 0xff, p);
    emitByte(jump & 0xff, p);
}

static void emitReturn(Parser* p)
{
    emitConstant(NIL_CONSTANT, p);
    emitBytes(OP_RETURN, 1, p);
}

static size_t getBytePos(Parser* p)
{
    return currentChunk(p)->count;
}

static void beginScope(Parser* p)
{
    p->compiler->currentScope++;
}

static void endScope(Parser* p)
{
    p->compiler->currentScope--;

    while (p->compiler->localCount > 0 &&
      p->compiler->locals[p->compiler->localCount - 1].scope > p->compiler->currentScope)
    {
        p->compiler->locals[p->compiler->localCount - 1].isCaptured ? emitByte(OP_CLOSE_UPVALUE, p)
                                                                    : emitByte(OP_POP, p);
        p->compiler->localCount--;
    }
}

static void endScopeUntil(int targetScope, Parser* p)
{
    size_t i = p->compiler->localCount;

    while (i > 0 && p->compiler->locals[i - 1].scope > targetScope)
    {
        p->compiler->locals[i - 1].isCaptured ? emitByte(OP_CLOSE_UPVALUE, p) : emitByte(OP_POP, p);
        i--;
    }
}

static void beginLoop(Parser* p)
{
    if (p->compiler->currentLoopScope + 1 > UINT8_MAX)
    {
        error("Error, inner loop depth exceeded.", p);
    }

    p->compiler->currentLoopScope++;
    size_t* loopContext = &p->compiler->loopContexts[p->compiler->currentLoopScope];
    *loopContext = p->compiler->currentScope;
}

static void endLoop(Parser* p)
{
    p->compiler->currentLoopScope--;

    while (p->compiler->breakCount > 0 &&
      p->compiler->breaks[p->compiler->breakCount - 1].depth > p->compiler->currentLoopScope)
    {
        Break* br = &p->compiler->breaks[p->compiler->breakCount - 1];
        patchJump(br->pos, p);
        p->compiler->breakCount--;
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
static ObjFunction* endCompiler(Parser* p)
{
    emitReturn(p);

    ObjFunction* function = p->compiler->function;
    p->compiler = p->compiler->enclosing;

    return function;
}

// common declaration
static void parse(Precedence prevPrec, ExpDesc* e, Parser* p);
static bool statements(Parser* p);
static void functionStatement(bool local, Parser* p);
static void functionBody(Parser* p);
static void expression(ExpDesc* e, Parser* p);
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

/*
   Mark the variable at `localIndex` as initialized by changing its scope to the current scope.
*/
static void markInitialized(uint8_t localIndex, Parser* p)
{
    p->compiler->locals[localIndex].scope = p->compiler->currentScope;
}

static uint8_t addLocal(Token* name, Parser* p)
{
    if (p->compiler->localCount + 1 > UINT8_MAX)
    {
        error("Error, too many local variables declared.", p);
        return 0;
    }

    Local* local = &p->compiler->locals[p->compiler->localCount];
    local->start = name->start;
    local->length = name->length;
    local->scope = -1;
    local->isCaptured = false;

    p->compiler->localCount++;
    return p->compiler->localCount - 1;
}

static int addUpvalue(uint8_t index, bool immediate, Compiler* c)
{
    int upvalueCount = c->upvalueCount;

    for (int i = 0; i < upvalueCount; i++)
    {
        Upvalue* upvalue = &c->upvalues[i];

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

    Upvalue* upvalue = &c->upvalues[upvalueCount];
    upvalue->index = index;
    upvalue->immediate = immediate;

    c->upvalueCount++;

    return upvalueCount;
}

static int lookupUpvalue(Token* name, Compiler* c)
{
    if (c->enclosing == NULL)
    {
        return -1;
    }

    int index = lookupLocal(name, c->enclosing);

    if (index != -1)
    {
        // add upvalue to the closure that actual needs the upvalue, not where it is found
        c->enclosing->locals[index].isCaptured = true;
        return addUpvalue((uint8_t)index, true, c);
    }

    index = lookupUpvalue(name, c->enclosing);
    if (index != -1)
    {
        return addUpvalue((uint8_t)index, false, c);
    }

    return -1;
}

static void namedVariable(ExpDesc* e, LhsAssign* lhs, Parser* p)
{
    Token name = p->prev;
    uint8_t var_constant = identifierConstant(&name, p);

    uint8_t opGet;
    ExpKind k = EXP_LOCAL;
    int index = lookupLocal(&name, p->compiler);

    if (index != -1)
    {
        opGet = OP_GET_LOCAL;
        k = EXP_LOCAL;
    }
    else if ((index = lookupUpvalue(&name, p->compiler)) != -1)
    {
        opGet = OP_GET_UPVALUE;
        k = EXP_UPVAL;
    }
    else
    {
        opGet = OP_GET_GLOBAL;
        index = var_constant;
        k = EXP_GLOBAL;
    }

    e->kind = k;

    if (lhs == NULL)
    {
        emitBytes(opGet, (uint8_t)index, p);
        e->patch = currentChunk(p)->count - 2;
    }
    else
    {
        lhs->e.kind = k;
        lhs->index = (uint8_t)index;
    }
}

static void unary(ExpDesc* e, Parser* p)
{
    TokenType op = prev(p);

    // all negate operators are right associative
    parse(PREC_UNARY, e, p);

    switch (op)
    {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE, p);
            break;
        case TOKEN_NOT:
            emitByte(OP_NOT, p);
            break;
        case TOKEN_HASH:
            emitByte(OP_LENGTH, p);
            break;
        default:
            error("Erorr, uknown operator found.", p);
    }
}

static void binary(ExpDesc* e, Parser* p)
{
    TokenType op = prev(p);
    Rule* rule = getRule(op);

    // right associative
    if (op == TOKEN_CARET)
    {
        parse(rule->prec, e, p);
    }
    // left associative
    else
    {
        parse(rule->prec + 1, e, p);
    }

    if (op == TOKEN_PLUS)
    {
        emitByte(OP_ADD, p);
    }
    else if (op == TOKEN_MINUS)
    {
        emitByte(OP_MINUS, p);
    }
    else if (op == TOKEN_STAR)
    {
        emitByte(OP_MUL, p);
    }
    else if (op == TOKEN_SLASH)
    {
        emitByte(OP_DIV, p);
    }
    else if (op == TOKEN_CARET)
    {
        emitByte(OP_EXPONENT, p);
    }
    else if (op == TOKEN_PERCENT)
    {
        emitByte(OP_MODULO, p);
    }
    else if (op == TOKEN_DOT_DOT)
    {
        emitByte(OP_JOIN, p);
    }
    else
    {
        fprintf(stderr, "Error, cannot parse binary operator.\n");
        exit(EXIT_FAILURE);
    }
}

static void number(ExpDesc* e, Parser* p)
{
    double num = strtod(p->prev.start, NULL);
    Value value = NUM_VAL(num);
    emitConstant(value, p);
    e->kind = EXP_NUM;
}

static void bool_and_nil(ExpDesc* e, Parser* p)
{
    if (prev(p) == TOKEN_TRUE)
    {
        emitConstant(BOOL_VAL(true), p);
        e->kind = EXP_TRUE;
    }
    else if (prev(p) == TOKEN_FALSE)
    {
        emitConstant(BOOL_VAL(false), p);
        e->kind = EXP_FALSE;
    }
    else
    {
        emitConstant(NIL_VAL(), p);
        e->kind = EXP_NIL;
    }
}

static void str(ExpDesc* e, Parser* p)
{
    const char* text = p->prev.start + 1;
    size_t length = p->prev.length - 2;

    ObjString* string = copyString(text, length, p->strings);
    emitConstant(OBJ_VAL(string), p);
    e->kind = EXP_STR;
}

static void block(const char* message, Parser* p)
{
    while (peek(p) != TOKEN_END && !isAtEnd(p))
    {
        statements(p);
    }

    consume(TOKEN_END, message, p);
}

static void relational(ExpDesc* e, Parser* p)
{
    TokenType op = prev(p);
    parse(PREC_RELATIONAL + 1, e, p);

    if (op == TOKEN_LESS)
    {
        emitByte(OP_LESS, p);
    }
    else if (op == TOKEN_LESS_EQUAL)
    {
        emitByte(OP_GREATER, p);
        emitByte(OP_NOT, p);
    }
    else if (op == TOKEN_GREATER)
    {
        emitByte(OP_GREATER, p);
    }
    else if (op == TOKEN_GREATER_EQUAL)
    {
        emitByte(OP_LESS, p);
        emitByte(OP_NOT, p);
    }
    else if (op == TOKEN_EQUAL_EQUAL)
    {
        emitByte(OP_EQUAL, p);
    }
    else
    {
        emitByte(OP_EQUAL, p);
        emitByte(OP_NOT, p);
    }
}

/*
 * `or` operator leaves the first truthful value evaluated on the stack
 * */
static void logical_or(ExpDesc* e, Parser* p)
{
    size_t nextBranchJump = emitJump(OP_JUMP_IF_FALSE, p);
    size_t endJump = emitJump(OP_JUMP, p);

    patchJump(nextBranchJump, p);
    emitByte(OP_POP, p);

    parse(PREC_OR, e, p);

    patchJump(endJump, p);
}

/*
 * `and` operator leaves the last value evaluated on the stack
 * */
static void logical_and(ExpDesc* e, Parser* p)
{
    size_t endJump = emitJump(OP_JUMP_IF_FALSE, p);

    emitByte(OP_POP, p);
    parse(PREC_AND, e, p);

    patchJump(endJump, p);
}

static uint8_t expressionList(ExpDesc* e, Parser* p)
{
    uint8_t length = 0;

    do
    {
        expression(e, p);

        if (length > UINT8_MAX)
        {
            error("Error, argument count exceeded.", p);
        }

        length++;
    } while (match(TOKEN_COMMA, p));

    if (HAS_MULTRET(e->kind))
    {
        patchSingleByte(e->patch, MULTRET, p);
    }

    return length;
}

static void evaluateAssign(LhsAssign* lhs, bool assign, Parser* p)
{
    if (lhs == NULL)
    {
        return;
    }

    switch (lhs->e.kind)
    {
        case EXP_LOCAL:
            emitBytes(assign ? OP_SET_LOCAL : OP_GET_LOCAL, lhs->index, p);
            break;
        case EXP_UPVAL:
            emitBytes(assign ? OP_SET_UPVALUE : OP_GET_UPVALUE, lhs->index, p);
            break;
        case EXP_INDEX:
            emitByte(assign ? OP_SET_FIELD : OP_GET_FIELD, p);
            break;
        case EXP_GLOBAL:
            emitBytes(assign ? OP_SET_GLOBAL : OP_GET_GLOBAL, lhs->index, p);
            break;
        default:
            return;
    }
}

static int field(int expIdx, Parser* p)
{
    /*
        field ::= `[´ exp `]´ `=´ exp | Name `=´ exp | exp
    */
    ExpDesc e;
    initExpDesc(&e);

    switch (peek(p))
    {
        case TOKEN_LEFT_SQUARE:
            advance(p);

            // key
            expression(&e, p);

            consume(TOKEN_RIGHT_SQUARE, "Error, missing ']' for field construction.", p);
            consume(TOKEN_EQUAL,
              "Error, missing '=' character for field construction of form [exp] = exp.", p);

            // value
            expression(&e, p);

            return 0;
        case TOKEN_IDENTIFIER:
        {
            Token name = p->current;
            advance(p);

            /* identifier = expr */
            if (match(TOKEN_EQUAL, p))
            {
                // key
                size_t nameConstant = identifierConstant(&name, p);
                emitBytes(OP_CONSTANT, nameConstant, p);

                // value
                expression(&e, p);

                return 0;
            }
            /* prefixexpr / functioncall */
            else
            {
                // key
                emitConstant(NUM_VAL(expIdx), p);

                // value
                expression(&e, p);

                return 1;
            }
        }
        default:
            // key
            emitConstant(NUM_VAL(expIdx), p);

            // value
            expression(&e, p);

            return 1;
    }
}

static void constructor(ExpDesc* e, Parser* p)
{
    /*
        tableconstructor ::= `{´ [fieldlist] `}´
        fieldlist ::= field {fieldsep field} [fieldsep]
        fieldsep ::= `,´ | `;´
    */

    consume(TOKEN_LEFT_BRACE, "Error, expect '{' for table constructor.", p);

    int expIdx = 1;
    int arity = 0;

    if (!check(TOKEN_RIGHT_BRACE, p))
    {
        do
        {
            if (field(expIdx, p) == 1)
            {
                expIdx++;
            }
            arity++;

            if (arity > 255)
            {
                error("Can't have more than 255 arguments.", p);
                break;
            }
        } while (match(TOKEN_COMMA, p) || match(TOKEN_SEMICOLON, p));
    }

    consume(TOKEN_RIGHT_BRACE, "Error, missing '}' to close constructor.", p);

    emitBytes(OP_CONSTRUCT, arity, p);
}

static uint8_t functionArguments(ExpDesc* e, Parser* p)
{
    uint8_t arity = 0;
    if (!check(TOKEN_RIGHT_PAREN, p))
    {
        arity = expressionList(e, p);
    }

    consume(TOKEN_RIGHT_PAREN, "Error, missing ')' to close function call.", p);

    return arity;
}

static void call(ExpDesc* e, Parser* p)
{
    uint8_t arity;

    switch (current(p))
    {
        case TOKEN_STRING:
            advance(p);
            str(e, p);
            arity = 1;
            break;
        case TOKEN_LEFT_BRACE:
            constructor(e, p);
            arity = 1;
            break;
        case TOKEN_LEFT_PAREN:
            advance(p);
            arity = functionArguments(e, p);
            break;
        default:
            advance(p);
            arity = 0;
            break;
    }

    // all functions will be restricted to returning only 1 value at first.
    // if a function is permitted to have more than 1 return value, the return value will be patched
    emitBytesABC(OP_CALL, arity, SINGLERET, p);
    e->kind = EXP_CALL;
    e->patch = getBytePos(p) - 1;
}

// pratt parsing table
Rule rules[] = { [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
    [TOKEN_MINUS] = { unary, binary, PREC_TERM },
    [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
    [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
    [TOKEN_PERCENT] = { NULL, binary, PREC_FACTOR },
    [TOKEN_CARET] = { NULL, binary, PREC_EXPONENT },
    [TOKEN_HASH] = { unary, NULL, PREC_NONE },
    [TOKEN_LESS] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_GREATER] = { NULL, relational, PREC_RELATIONAL },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_PAREN] = { NULL, call, PREC_CALL },
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
    [TOKEN_FALSE] = { NULL, NULL, PREC_NONE },
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
    [TOKEN_TRUE] = { NULL, NULL, PREC_NONE },
    [TOKEN_UNTIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_IDENTIFIER] = { NULL, NULL, PREC_NONE },
    [TOKEN_STRING] = { NULL, NULL, PREC_NONE },
    [TOKEN_NUMBER] = { NULL, NULL, PREC_NONE },
    [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] = { NULL, NULL, PREC_NONE } };

Rule* getRule(TokenType op)
{
    return &rules[op];
}

static void prefixExpression(ExpDesc* e, LhsAssign* lhs, Parser* p)
{
    /*
       prefixexp ::= Name | `(` expr `)`
    */

    switch (p->current.type)
    {
        case TOKEN_IDENTIFIER:
            advance(p);
            namedVariable(e, lhs, p);
            break;
        case TOKEN_LEFT_PAREN:
            advance(p);
            expression(e, p);
            consume(TOKEN_RIGHT_PAREN, "Error, '(' not closed, missing ')' character.", p);

            if (HAS_MULTRET(e->kind))
            {
                patchSingleByte(e->patch, SINGLERET, p);
            }

            e->kind = EXP_NIL;
            break;
        default:
            error("Error, unknown symbol encountered.", p);
            return;
    }
}

static void primaryExpression(ExpDesc* e, LhsAssign* lhs, bool def, Parser* p)
{
    // prefixExp ::= var | functioncall
    prefixExpression(e, lhs, p);

    while (true)
    {
        switch (current(p))
        {
            // `[` exp `]` | `.` Name
            case TOKEN_LEFT_SQUARE:
            case TOKEN_DOT:
            {
                // evaluate prefix to get table
                if (lhs != NULL)
                {
                    evaluateAssign(lhs, false, p);
                }

                if (current(p) == TOKEN_LEFT_SQUARE)
                {
                    advance(p);

                    // key
                    expression(e, p);

                    consume(TOKEN_RIGHT_SQUARE, "Error, expect ']' to close index operation.", p);
                }
                else
                {
                    advance(p);

                    // key
                    consume(TOKEN_IDENTIFIER, "Error, expect a name after '.'", p);
                    Token name = p->prev;
                    size_t nameConstant = identifierConstant(&name, p);

                    emitBytes(OP_CONSTANT, nameConstant, p);
                }

                if (lhs == NULL)
                {
                    emitByte(OP_GET_FIELD, p);
                }
                else
                {
                    lhs->fieldCount++;
                    lhs->e.kind = EXP_INDEX;
                }

                e->kind = EXP_INDEX;
                e->patch = getBytePos(p) - 1;
                break;
            }
            // `(` [explist] `)` | `{` [fieldlist] `}` | string
            case TOKEN_STRING:
            case TOKEN_LEFT_BRACE:
            case TOKEN_LEFT_PAREN:
            {
                /*
                    parsing 'primary' for a definition, so no call
                */
                if (def)
                {
                    return;
                }

                // parse the lhs since its a call
                evaluateAssign(lhs, false, p);
                call(e, p);
                break;
            }
            // `:` Name args
            case TOKEN_COLON:
                // TODO: implement colon for function expr
                break;
            default:
                return;
        }
    }
}

static void simpleExpression(ExpDesc* e, Parser* p)
{
    switch (current(p))
    {
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NIL:
            advance(p);
            bool_and_nil(e, p);
            break;
        case TOKEN_NUMBER:
            advance(p);
            number(e, p);
            break;
        case TOKEN_STRING:
            advance(p);
            str(e, p);
            break;
        case TOKEN_FUNCTION:
            advance(p);
            functionBody(p);
            break;
        case TOKEN_LEFT_BRACE:
            constructor(e, p);
            break;
        case TOKEN_THREE_DOTS:
            advance(p);
            if (!ALLOW_VARARG(p->compiler->function->type))
            {
                error(
                  "Erorr, vararg expression `...` cannot be used outside of vararg function.", p);
                break;
            }

            // cancel the arg local variable
            p->compiler->function->type = TYPE_VARARG_NO_ARG;

            emitBytes(OP_VARARG, SINGLERET, p);
            e->kind = EXP_VARARG;
            e->patch = getBytePos(p) - 1;
            break;
        default:
            primaryExpression(e, NULL, false, p);
            break;
    }
}

static void parse(Precedence prevPrec, ExpDesc* e, Parser* p)
{
    p->currentPrec = prevPrec;

    TokenType prefixOp = current(p);
    ParseFn prefixFn = getRule(prefixOp)->prefix;

    if (prefixFn != NULL)
    {
        advance(p);
        prefixFn(e, p);
    }
    else
    {
        simpleExpression(e, p);
    }

    Rule* infixRule;
    while ((infixRule = getRule(peek(p)))->prec >= prevPrec)
    {
        // move to the next prefix starting point
        advance(p);
        p->currentPrec = infixRule->prec;

        ParseFn infixFn = infixRule->infix;

        if (infixFn == NULL)
        {
            fprintf(stderr, "Error, missing infix rule.\n");
        }

        infixFn(e, p);
    }

    p->currentPrec = prevPrec;
}

static void expression(ExpDesc* e, Parser* p)
{
    parse(PREC_ASSIGNMENT, e, p);
}

static void adjustAssign(ExpDesc* last, uint8_t nexprs, uint8_t expected, Parser* p)
{
    uint8_t diff = abs(expected - nexprs);

    if (HAS_MULTRET(last->kind))
    {
        if (diff == 0)
        {
            patchSingleByte(last->patch, SINGLERET, p);
        }
        else if (nexprs < expected)
        {
            /* return value offset by 1 */
            patchSingleByte(last->patch, MAKE_RET(diff + 1), p);
            return;
        }
        else
        {
            patchSingleByte(last->patch, ZERORET, p);
            diff--;
            for (uint8_t i = 0; i < diff; i++)
            {
                emitByte(OP_POP, p);
            }
        }
    }
    else
    {
        for (uint8_t i = 0; i < diff; i++)
        {
            if (nexprs > expected)
            {
                emitByte(OP_POP, p);
            }
            else
            {
                emitConstant(NIL_CONSTANT, p);
            }
        }
    }
}

inline static uint8_t swapGetAndSet(ExpDesc* e, Parser* p)
{
    switch (e->kind)
    {
        case EXP_LOCAL:
            return OP_SET_LOCAL;
        case EXP_UPVAL:
            return OP_SET_UPVALUE;
        case EXP_INDEX:
            return 0;
        case EXP_GLOBAL:
            return OP_SET_GLOBAL;
        default:
            error("Error, no assignment target found.", p);
            return 0;
    }
}

static void assign(int nvars, Parser* p)
{
    LhsAssign lhs;
    lhs.fieldCount = 0;
    initExpDesc(&lhs.e);

    if (match(TOKEN_COMMA, p))
    {
        primaryExpression(&lhs.e, &lhs, false, p);

        if (!HAS_ASSIGN(lhs.e.kind))
        {
            error("Error, cannot assign to this target.", p);
        }

        assign(nvars + 1, p);
    }
    else
    {
        ExpDesc e;
        initExpDesc(&e);

        consume(TOKEN_EQUAL, "Error, missing '=' for assignment.", p);

        uint8_t nexprs = expressionList(&e, p);
        adjustAssign(&e, nexprs, nvars, p);

        emitBytes(OP_CACHE, (uint8_t)nvars, p);

        return;
    }

    // generate code from lhs
    evaluateAssign(&lhs, true, p);
}

static void expressionStatement(Parser* p)
{
    /*
       stat ::= varlist `=` explist |
                functioncall
    */

    ExpDesc e;
    initExpDesc(&e);

    LhsAssign lhs;
    lhs.fieldCount = 0;
    initExpDesc(&lhs.e);

    size_t firstVar = getBytePos(p);
    primaryExpression(&e, &lhs, false, p);

    if (e.kind == EXP_CALL)
    {
        patchSingleByte(e.patch, ZERORET, p);
        return;
    }
    else
    {
        int nvars = 1;
        assign(nvars, p);

        // evaluate the first expression
        evaluateAssign(&lhs, true, p);
    }
}

static uint8_t defineLocalVar(Token* name, Parser* p)
{
    uint8_t constantIndex = identifierConstant(name, p);
    uint8_t localIndex = addLocal(name, p);

    return localIndex;
}

static uint8_t defineReservedVar(ReservedVarType type, Parser* p)
{
    Token var = genReservedVarToken(type);
    uint8_t varIdx = defineLocalVar(&var, p);

    return varIdx;
}

static void paramList(Parser* p)
{
    if (!check(TOKEN_RIGHT_PAREN, p))
    {
        do
        {
            p->compiler->function->arity++;
            if (p->compiler->function->arity > UINT8_MAX)
            {
                error("Error, can't have more than 255 parameters.", p);
            }

            switch (current(p))
            {
                case TOKEN_IDENTIFIER:
                {
                    Token name = p->current;
                    advance(p);
                    uint8_t constant = defineLocalVar(&name, p);
                    markInitialized(constant, p);
                    break;
                }
                case TOKEN_THREE_DOTS:
                {
                    advance(p);
                    Token argToken = genReservedVarToken(RESERVED_ARG);
                    uint8_t constant = defineLocalVar(&argToken, p);
                    markInitialized(constant, p);

                    p->compiler->function->type = TYPE_VARARG;

                    if (!check(TOKEN_RIGHT_PAREN, p))
                    {
                        error("Error, vararg must be at the end of parameter list.", p);
                    }
                    break;
                }
                default:
                    error("Error, invalid expression in parameter list.", p);
                    break;
            }
        } while (match(TOKEN_COMMA, p));
    }

    consume(TOKEN_RIGHT_PAREN, "Error, expect ')' after argument list.", p);
}

static void functionBody(Parser* p)
{
    consume(TOKEN_LEFT_PAREN, "Error, expect '(' after.", p);

    Compiler compiler;
    initCompiler(&compiler, p->compiler);

    beginScope(p);

    /* start parsing current function */
    p->compiler = &compiler;

    /* parse parameters */
    paramList(p);

    block("Error, expect 'end' after function definition.", p);

    // endCompiler emits a return that would already reset the stack, no need for endScope
    ObjFunction* function = endCompiler(p);
    endScope(p);

    function->upvalueCount = compiler.upvalueCount;

    /* finish parsing current function */

    ObjClosure* closure = newClosure(function);

    size_t constant = addConstant(currentChunk(p), OBJ_VAL(function));

    emitBytes(OP_CLOSURE, constant, p);

    // add upvalues
    for (int i = 0; i < function->upvalueCount; i++)
    {
        Upvalue* upvalue = &compiler.upvalues[i];

        emitByte(upvalue->immediate ? 1 : 0, p);
        emitByte((uint8_t)upvalue->index, p);
    }
}

/*
Generate a `ObjFunction` object that represents the function currently defined. If local is true,
then the function is defined locally with a single identifier as an assign target. If not, the
function can have a table field or a global variable as the assign target.

Parameters:
- `local`: The option to denote whether this function is defined locally or not
*/
static void functionStatement(bool local, Parser* p)
{
    LhsAssign lhs;
    lhs.fieldCount = 0;
    initExpDesc(&lhs.e);

    if (!check(TOKEN_IDENTIFIER, p))
    {
        error("Error, expect function name after 'function' and before arguments.", p);
    }

    /* Name */
    if (local)
    {
        Token name = p->current;
        advance(p);
        uint8_t local = defineLocalVar(&name, p);
        markInitialized(local, p);
        lhs.e.kind = EXP_LOCAL;
    }
    /* Name | prefix */
    else
    {
        primaryExpression(&lhs.e, &lhs, true, p);
    }

    functionBody(p);

    // set assign if needed
    if (!local)
    {
        emitBytes(OP_CACHE, (uint8_t)1, p);
        evaluateAssign(&lhs, true, p);
    }
}

static void localVarStatement(Parser* p)
{
    int nvars = 0;
    int nexprs = 0;
    uint8_t initIdx = p->compiler->localCount;

    ExpDesc e;
    initExpDesc(&e);

    do
    {
        consume(TOKEN_IDENTIFIER, "Error, an identifier is required.", p);
        Token name = p->prev;
        defineLocalVar(&name, p);

        if (nvars + 1 >= UINT8_MAX)
        {
            error("Error ,too many variables in assignment statement.", p);
            return;
        }

        nvars++;
    } while (match(TOKEN_COMMA, p));

    if (match(TOKEN_EQUAL, p))
    {
        nexprs = expressionList(&e, p);
    }
    else
    {
        for (int i = 0; i < nvars; i++)
        {
            emitConstant(NIL_CONSTANT, p);
        }
        nexprs = nvars;
    }

    for (int i = 0; i < nvars; i++)
    {
        markInitialized(initIdx + i, p);
    }

    adjustAssign(&e, nexprs, nvars, p);
}

static void ifStatement(Parser* p)
{
    ExpDesc e;
    initExpDesc(&e);

    // start of a single if branch
    expression(&e, p);
    consume(TOKEN_THEN, "Error, expect 'then' to follow after condition expression.", p);

    size_t nextBranchJump = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);

    // body statements
    beginScope(p);
    while (!isAtEnd(p) && peek(p) != TOKEN_ELSE && peek(p) != TOKEN_ELSEIF && peek(p) != TOKEN_END)
    {
        statements(p);
    }
    endScope(p);
    size_t endJump = emitJump(OP_JUMP, p);

    patchJump(nextBranchJump, p);
    emitByte(OP_POP, p);

    // possible parralel branches
    if (match(TOKEN_ELSEIF, p))
    {
        ifStatement(p);
    }
    else if (match(TOKEN_ELSE, p))
    {
        beginScope(p);
        block("Error, missing token 'end' to close if statement.", p);
        endScope(p);
    }
    else
    {
        consume(TOKEN_END, "Error, missing token 'end' to close if statement.", p);
    }

    // jump to end of if-statement
    patchJump(endJump, p);
}

static void whileStatement(Parser* p)
{
    ExpDesc e;
    initExpDesc(&e);

    beginLoop(p);
    size_t loopStart = getBytePos(p);

    expression(&e, p);
    size_t endJump = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);

    consume(TOKEN_DO, "Error, expect token 'do' after while statement.", p);
    beginScope(p);
    block("Error, missing token 'end' to close while statement", p);
    endScope(p);

    emitBackJump(OP_LOOP, loopStart, p);

    patchJump(endJump, p);
    emitByte(OP_POP, p);
    endLoop(p);
}

static void repeatStatement(Parser* p)
{
    ExpDesc e;
    initExpDesc(&e);

    beginLoop(p);
    size_t loopStart = getBytePos(p);

    beginScope(p);
    while (!isAtEnd(p) && peek(p) != TOKEN_UNTIL)
    {
        statements(p);
    }
    consume(TOKEN_UNTIL, "Error, expect until before condition of repeat statement.", p);
    expression(&e, p);
    // flip condition to loop while exit condition is false
    emitByte(OP_NOT, p);
    endScope(p);

    size_t endJump = emitJump(OP_JUMP_IF_FALSE, p);

    emitByte(OP_POP, p);
    emitBackJump(OP_LOOP, loopStart, p);

    patchJump(endJump, p);
    emitByte(OP_POP, p);

    endLoop(p);
}

static void breakStatement(Parser* p)
{
    if (p->compiler->currentLoopScope == 0)
    {
        errorAt(p->prev, "Error, a break statement cannot appear outside of a loop.", p);
    }
    else if (p->compiler->breakCount + 1 > UINT8_MAX)
    {
        error("Error, too many break statement inside a single block.", p);
    }

    // clean up until we meet the outer scope of the loop
    size_t loopScope = p->compiler->loopContexts[p->compiler->currentLoopScope];

    endScopeUntil(loopScope, p);

    size_t breakJump = emitJump(OP_JUMP, p);
    Break* br = &p->compiler->breaks[p->compiler->breakCount];
    br->pos = breakJump;
    br->depth = p->compiler->currentLoopScope;
    p->compiler->breakCount++;
}

static void returnStatement(Parser* p)
{
    ExpDesc e;
    initExpDesc(&e);

    uint8_t nexprs = 1;

    // return statement now carry with it the number of expression it would return
    // if this number does not match with the number permitted by the call, adjustment is needed

    if (blockFollow(current(p)))
    {
        emitConstant(NIL_CONSTANT, p);
    }
    else
    {
        nexprs = expressionList(&e, p);
    }

    emitBytes(OP_RETURN, nexprs, p);
}

static void numericalFor(Token* loop_var, Parser* p)
{
    ExpDesc e;
    initExpDesc(&e);

    beginScope(p);

    // initializer
    expression(&e, p);
    consume(TOKEN_COMMA, "Error, no comma to separate between iniitializer, limit and step.", p);

    // limit
    expression(&e, p);

    // step
    if (match(TOKEN_COMMA, p))
    {
        expression(&e, p);
    }
    else
    {
        emitConstant(NUM_VAL(1), p);
    }

    /* define temporary variables */
    uint8_t varIdx = defineReservedVar(RESERVED_VAR, p);
    uint8_t limIdx = defineReservedVar(RESERVED_LIMIT, p);
    uint8_t stepIdx = defineReservedVar(RESERVED_STEP, p);
    markInitialized(varIdx, p);
    markInitialized(limIdx, p);
    markInitialized(stepIdx, p);

    size_t zeroConstant = addConstant(currentChunk(p), NUM_VAL(0));

    beginLoop(p);
    size_t loopStart = getBytePos(p);

    // start condition

    /*
       (step > 0 and var <= limit)
    */
    emitBytes(OP_GET_LOCAL, stepIdx, p);
    emitBytes(OP_CONSTANT, (uint8_t)zeroConstant, p);
    emitByte(OP_GREATER, p);
    size_t andJump1 = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);
    emitBytes(OP_GET_LOCAL, varIdx, p);
    emitBytes(OP_GET_LOCAL, limIdx, p);
    emitBytes(OP_GREATER, OP_NOT, p);
    patchJump(andJump1, p);

    /*
       or
    */
    size_t orNextJump = emitJump(OP_JUMP_IF_FALSE, p);
    size_t orEndJump = emitJump(OP_JUMP, p);

    patchJump(orNextJump, p);
    emitByte(OP_POP, p);

    /*
       (step <= 0 and var >= limit)
    */
    emitBytes(OP_GET_LOCAL, stepIdx, p);
    emitBytes(OP_CONSTANT, (uint8_t)zeroConstant, p);
    emitBytes(OP_GREATER, OP_NOT, p);
    size_t andJump2 = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);
    emitBytes(OP_GET_LOCAL, varIdx, p);
    emitBytes(OP_GET_LOCAL, limIdx, p);
    emitBytes(OP_NOT, OP_LESS, p);
    patchJump(andJump2, p);
    patchJump(orEndJump, p);

    // end condition

    // start body

    size_t endLoopJump = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);

    consume(TOKEN_DO, "Error, expect 'do' after initializations.", p);
    beginScope(p);

    /*
       local v = var
    */
    uint8_t loopVarIdx = defineLocalVar(loop_var, p);
    emitBytes(OP_GET_LOCAL, varIdx, p);
    markInitialized(loopVarIdx, p);

    block("Error, missing 'end' to close for statement.", p);

    /*
       var = var + step
    */
    emitBytes(OP_GET_LOCAL, varIdx, p);
    emitBytes(OP_GET_LOCAL, stepIdx, p);
    emitByte(OP_ADD, p);
    emitBytes(OP_CACHE, 1, p);
    emitBytes(OP_SET_LOCAL, varIdx, p);

    endScope(p);

    // end body

    emitBackJump(OP_LOOP, loopStart, p);

    patchJump(endLoopJump, p);
    emitByte(OP_POP, p);
    endLoop(p);

    /* end loop*/

    endScope(p);
}

static void genericFor(Token* initial, Parser* p)
{
    beginScope(p);

    ExpDesc e;
    initExpDesc(&e);

    // declare reserved var
    uint8_t fIdx = defineReservedVar(RESERVED_FUNCTION, p);
    uint8_t sIdx = defineReservedVar(RESERVED_STATE, p);
    uint8_t varIdx = defineReservedVar(RESERVED_VAR, p);

    // declare local variables
    uint8_t var1Idx = defineLocalVar(initial, p);
    uint8_t length = 1;
    while (match(TOKEN_COMMA, p))
    {
        Token name = p->current;
        advance(p);
        defineLocalVar(&name, p);
        length++;
    }

    /*
       local f, s, var = explist
    */
    consume(TOKEN_IN, "Error, 'in' expected after name list.", p);
    uint8_t resnexprs = expressionList(&e, p);
    adjustAssign(&e, resnexprs, 3, p);
    markInitialized(fIdx, p);
    markInitialized(sIdx, p);
    markInitialized(varIdx, p);

    /* start of loop */
    beginLoop(p);
    beginScope(p);
    size_t loopStart = getBytePos(p);

    /*
       local var_1 ... var_n = f(s, var)
    */
    emitBytes(OP_GET_LOCAL, fIdx, p);
    emitBytes(OP_GET_LOCAL, sIdx, p);
    emitBytes(OP_GET_LOCAL, varIdx, p);
    emitBytesABC(OP_CALL, 2, MAKE_RET(length), p);

    /*
       var = var1
    */
    emitBytes(OP_GET_LOCAL, var1Idx, p);
    emitBytes(OP_CACHE, 1, p);
    emitBytes(OP_SET_LOCAL, varIdx, p);
    for (uint8_t i = 0; i < length; i++)
    {
        markInitialized(var1Idx + i, p);
    }

    /*
       if var == nil then break end
    */
    emitBytes(OP_GET_LOCAL, varIdx, p);
    emitConstant(NIL_CONSTANT, p);
    emitByte(OP_EQUAL, p);
    size_t elseJump = emitJump(OP_JUMP_IF_FALSE, p);
    emitByte(OP_POP, p);
    breakStatement(p);
    patchJump(elseJump, p);
    emitByte(OP_POP, p);

    // body
    consume(TOKEN_DO, "Error, expect 'do' after initializations.", p);
    block("Error, missing 'end' to close for statement.", p);
    endScope(p);

    emitBackJump(OP_LOOP, loopStart, p);

    endLoop(p);
    endScope(p);
}

static void forStatement(Parser* p)
{
    consume(TOKEN_IDENTIFIER, "Error, identifier needs to follow 'for' statement.", p);
    Token loop_var = p->prev;

    if (match(TOKEN_EQUAL, p))
    {
        numericalFor(&loop_var, p);
    }
    else if (peek(p) == TOKEN_IN || peek(p) == TOKEN_COMMA)
    {
        genericFor(&loop_var, p);
    }
    else
    {
        error("Error, unknown keyword found after 'for'.", p);
    }
}

static bool statements(Parser* p)
{
    if (match(TOKEN_LOCAL, p))
    {
        if (match(TOKEN_FUNCTION, p))
        {
            functionStatement(true, p);
        }
        else
        {
            localVarStatement(p);
        }
        return false;
    }
    else if (match(TOKEN_FUNCTION, p))
    {
        functionStatement(false, p);
        return false;
    }
    else if (match(TOKEN_DO, p))
    {
        beginScope(p);
        block("Error, missing token 'end' to close block.", p);
        endScope(p);
        return false;
    }
    else if (match(TOKEN_IF, p))
    {
        ifStatement(p);
        return false;
    }
    else if (match(TOKEN_WHILE, p))
    {
        whileStatement(p);
        return false;
    }
    else if (match(TOKEN_REPEAT, p))
    {
        repeatStatement(p);
        return false;
    }
    else if (match(TOKEN_BREAK, p))
    {
        breakStatement(p);
        return true;
    }
    else if (match(TOKEN_FOR, p))
    {
        forStatement(p);
        return false;
    }
    else if (match(TOKEN_RETURN, p))
    {
        returnStatement(p);
        return true;
    }
    else
    {
        expressionStatement(p);
        return false;
    }

    return true;
}

static void chunk(Parser* p)
{
    while (!isAtEnd(p))
    {
        bool stmt = statements(p);

        // consume an optinal semicolon
        match(TOKEN_SEMICOLON, p);
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
    chunk(&parser);

    if (parser.hadError)
    {
        fprintf(stderr, "Compiler error, exiting now.\n");
        exit(EXIT_FAILURE);
    }

    ObjFunction* function = endCompiler(&parser);
    return function;
}
