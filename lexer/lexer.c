#include <lexer.h>
#include <token.h>

#include <stdio.h>

static void error(const char* message, Lexer* lexer)
{
    fprintf(stderr, "Error at line %zu, character '%c'.\n", lexer->line, *lexer->current);
    fprintf(stderr, "Error reads: %s.", message);
}

void initLexer(const char* source, Lexer* lexer)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

bool isAtEnd(Lexer* lexer)
{
    return *lexer->current == '\0';
}

static char peek(Lexer* lexer)
{
    if (isAtEnd(lexer))
    {
        return '\0';
    }

    return *lexer->current;
}

static char advance(Lexer* lexer)
{
    if (isAtEnd(lexer))
    {
        return '\0';
    }

    return *lexer->current++;
}

static bool match(char target, Lexer* lexer)
{
    if (*lexer->current != target)
    {
        return false;
    }

    advance(lexer);
    return true;
}

static void skipWhitespace(Lexer* lexer)
{
    while (peek(lexer) == ' ' || peek(lexer) == '\t')
    {
        advance(lexer);
    }
}

Token makeToken(TokenType type, Lexer* lexer)
{
    Token token;

    token.type = type;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;

    return token;
}

Token lex(Lexer* lexer)
{
    lexer->start = lexer->current;

    char c = advance(lexer);

    switch (c)
    {
        case '+':
            return makeToken(TOKEN_PLUS, lexer);
        case '-':
            if (match('-', lexer))
            {
                skipWhitespace(lexer);

                if (match('[', lexer) && match('[', lexer))
                {
                    while (!isAtEnd(lexer) && peek(lexer) != ']')
                    {
                        if (advance(lexer) == '\n')
                        {
                            lexer->line++;
                        }
                    }

                    if (isAtEnd(lexer))
                    {
                        error("Multiline comment is not properly closed", lexer);
                    }

                    // skip closing braces and newline
                    advance(lexer);
                    advance(lexer);
                    advance(lexer);
                }
                else
                {
                    while (peek(lexer) != '\n')
                    {
                        advance(lexer);
                    }

                    // skip newline
                    advance(lexer);
                }

                return lex(lexer);
            }
            else
            {
                return makeToken(TOKEN_MINUS, lexer);
            }
        case '*':
            return makeToken(TOKEN_STAR, lexer);
        case '/':
            return makeToken(TOKEN_SLASH, lexer);
        case '%':
            return makeToken(TOKEN_PERCENT, lexer);
        case '^':
            return makeToken(TOKEN_CARET, lexer);
        case '#':
            return makeToken(TOKEN_HASH, lexer);
        case '<':
            if (match('=', lexer))
            {
                return makeToken(TOKEN_LESS_EQUAL, lexer);
            }
            else
            {
                return makeToken(TOKEN_LESS, lexer);
            }
        case '>':
            if (match('=', lexer))
            {
                return makeToken(TOKEN_GREATER_EQUAL, lexer);
            }
            else
            {
                return makeToken(TOKEN_GREATER, lexer);
            }
        case '=':
            if (match('=', lexer))
            {
                return makeToken(TOKEN_EQUAL_EQUAL, lexer);
            }
            else
            {
                return makeToken(TOKEN_EQUAL, lexer);
            }
        case '~':
            if (match('=', lexer))
            {
                return makeToken(TOKEN_TILDE_EQUAL, lexer);
            }
            else
            {
                return makeToken(TOKEN_ERROR, lexer);
            }
        case '(':
            return makeToken(TOKEN_LEFT_PAREN, lexer);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN, lexer);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE, lexer);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE, lexer);
        case '[':
            return makeToken(TOKEN_LEFT_SQUARE, lexer);
        case ']':
            return makeToken(TOKEN_RIGHT_SQUARE, lexer);
        case ';':
            return makeToken(TOKEN_SEMICOLON, lexer);
        case ':':
            return makeToken(TOKEN_COLON, lexer);
        case ',':
            return makeToken(TOKEN_COMMA, lexer);
        case '.':
            if (match('.', lexer))
            {
                if (match('.', lexer))
                {
                    return makeToken(TOKEN_THREE_DOTS, lexer);
                }
                else
                {
                    return makeToken(TOKEN_DOT_DOT, lexer);
                }
            }
            else
            {
                return makeToken(TOKEN_DOT, lexer);
            }
        case ' ':
        case '\t':
            skipWhitespace(lexer);
            return lex(lexer);
        case '\n':
            lexer->line++;
            return lex(lexer);
        case '\0':
            return makeToken(TOKEN_EOF, lexer);
        default:
            return makeToken(TOKEN_ERROR, lexer);
    }
}

#define DEBUG_PRINT
void printToken(Token* token)
{
    switch (token->type)
    {
        case TOKEN_PLUS:
            printf("[TOKEN_PLUS]");
            break;
        case TOKEN_MINUS:
            printf("[TOKEN_MINUS]");
            break;
        case TOKEN_STAR:
            printf("[TOKEN_STAR]");
            break;
        case TOKEN_SLASH:
            printf("[TOKEN_SLASH]");
            break;
        case TOKEN_PERCENT:
            printf("[TOKEN_PERCENT]");
            break;
        case TOKEN_CARET:
            printf("[TOKEN_CARET]");
            break;
        case TOKEN_HASH:
            printf("[TOKEN_HASH]");
            break;
        case TOKEN_LESS:
            printf("[TOKEN_LESS]");
            break;
        case TOKEN_GREATER:
            printf("[TOKEN_GREATER]");
            break;
        case TOKEN_EQUAL:
            printf("[TOKEN_EQUAL]");
            break;
        case TOKEN_LEFT_PAREN:
            printf("[TOKEN_LEFT_PAREN]");
            break;
        case TOKEN_RIGHT_PAREN:
            printf("[TOKEN_RIGHT_PAREN]");
            break;
        case TOKEN_LEFT_BRACE:
            printf("[TOKEN_LEFT_BRACE]");
            break;
        case TOKEN_RIGHT_BRACE:
            printf("[TOKEN_RIGHT_BRACE]");
            break;
        case TOKEN_LEFT_SQUARE:
            printf("[TOKEN_LEFT_SQUARE]");
            break;
        case TOKEN_RIGHT_SQUARE:
            printf("[TOKEN_RIGHT_SQUARE]");
            break;
        case TOKEN_SEMICOLON:
            printf("[TOKEN_SEMICOLON]");
            break;
        case TOKEN_COLON:
            printf("[TOKEN_COLON]");
            break;
        case TOKEN_COMMA:
            printf("[TOKEN_COMMA]");
            break;
        case TOKEN_DOT:
            printf("[TOKEN_DOT]");
            break;
        case TOKEN_EQUAL_EQUAL:
            printf("[TOKEN_EQUAL_EQUAL]");
            break;
        case TOKEN_TILDE_EQUAL:
            printf("[TOKEN_TILDE_EQUAL]");
            break;
        case TOKEN_LESS_EQUAL:
            printf("[TOKEN_LESS_EQUAL]");
            break;
        case TOKEN_GREATER_EQUAL:
            printf("[TOKEN_GREATER_EQUAL]");
            break;
        case TOKEN_DOT_DOT:
            printf("[TOKEN_DOT_DOT]");
            break;
        case TOKEN_THREE_DOTS:
            printf("[TOKEN_THREE_DOTS]");
            break;
        case TOKEN_AND:
            printf("[TOKEN_AND]");
            break;
        case TOKEN_BREAK:
            printf("[TOKEN_BREAK]");
            break;
        case TOKEN_DO:
            printf("[TOKEN_DO]");
            break;
        case TOKEN_ELSE:
            printf("[TOKEN_ELSE]");
            break;
        case TOKEN_ELSEIF:
            printf("[TOKEN_ELSEIF]");
            break;
        case TOKEN_END:
            printf("[TOKEN_END]");
            break;
        case TOKEN_FALSE:
            printf("[TOKEN_FALSE]");
            break;
        case TOKEN_FOR:
            printf("[TOKEN_FOR]");
            break;
        case TOKEN_FUNCTION:
            printf("[TOKEN_FUNCTION]");
            break;
        case TOKEN_IF:
            printf("[TOKEN_IF]");
            break;
        case TOKEN_IN:
            printf("[TOKEN_IN]");
            break;
        case TOKEN_LOCAL:
            printf("[TOKEN_LOCAL]");
            break;
        case TOKEN_NIL:
            printf("[TOKEN_NIL]");
            break;
        case TOKEN_NOT:
            printf("[TOKEN_NOT]");
            break;
        case TOKEN_OR:
            printf("[TOKEN_OR]");
            break;
        case TOKEN_REPEAT:
            printf("[TOKEN_REPEAT]");
            break;
        case TOKEN_RETURN:
            printf("[TOKEN_RETURN]");
            break;
        case TOKEN_THEN:
            printf("[TOKEN_THEN]");
            break;
        case TOKEN_TRUE:
            printf("[TOKEN_TRUE]");
            break;
        case TOKEN_UNTIL:
            printf("[TOKEN_UNTIL]");
            break;
        case TOKEN_WHILE:
            printf("[TOKEN_WHILE]");
            break;
        case TOKEN_IDENTIFIER:
            printf("[TOKEN_IDENTIFIER]");
            break;
        case TOKEN_STRING:
            printf("[TOKEN_STRING]");
            break;
        case TOKEN_NUMBER:
            printf("[TOKEN_NUMBER]");
            break;
        case TOKEN_EOF:
            printf("[TOKEN_EOF]");
            break;
        case TOKEN_ERROR:
            break;
    }
}
#undef DEBUG_PRINT
