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

#ifdef DEBUG_PRINT_TOKEN
string convertToken(Token* token)
{
    switch (token->type)
    {
        case TOKEN_PLUS:
            return "[TOKEN_PLUS]";
            break;
        case TOKEN_MINUS:
            return "[TOKEN_MINUS]";
            break;
        case TOKEN_STAR:
            return "[TOKEN_STAR]";
            break;
        case TOKEN_SLASH:
            return "[TOKEN_SLASH]";
            break;
        case TOKEN_PERCENT:
            return "[TOKEN_PERCENT]";
            break;
        case TOKEN_CARET:
            return "[TOKEN_CARET]";
            break;
        case TOKEN_HASH:
            return "[TOKEN_HASH]";
            break;
        case TOKEN_LESS:
            return "[TOKEN_LESS]";
            break;
        case TOKEN_GREATER:
            return "[TOKEN_GREATER]";
            break;
        case TOKEN_EQUAL:
            return "[TOKEN_EQUAL]";
            break;
        case TOKEN_LEFT_PAREN:
            return "[TOKEN_LEFT_PAREN]";
            break;
        case TOKEN_RIGHT_PAREN:
            return "[TOKEN_RIGHT_PAREN]";
            break;
        case TOKEN_LEFT_BRACE:
            return "[TOKEN_LEFT_BRACE]";
            break;
        case TOKEN_RIGHT_BRACE:
            return "[TOKEN_RIGHT_BRACE]";
            break;
        case TOKEN_LEFT_SQUARE:
            return "[TOKEN_LEFT_SQUARE]";
            break;
        case TOKEN_RIGHT_SQUARE:
            return "[TOKEN_RIGHT_SQUARE]";
            break;
        case TOKEN_SEMICOLON:
            return "[TOKEN_SEMICOLON]";
            break;
        case TOKEN_COLON:
            return "[TOKEN_COLON]";
            break;
        case TOKEN_COMMA:
            return "[TOKEN_COMMA]";
            break;
        case TOKEN_DOT:
            return "[TOKEN_DOT]";
            break;
        case TOKEN_EQUAL_EQUAL:
            return "[TOKEN_EQUAL_EQUAL]";
            break;
        case TOKEN_TILDE_EQUAL:
            return "[TOKEN_TILDE_EQUAL]";
            break;
        case TOKEN_LESS_EQUAL:
            return "[TOKEN_LESS_EQUAL]";
            break;
        case TOKEN_GREATER_EQUAL:
            return "[TOKEN_GREATER_EQUAL]";
            break;
        case TOKEN_DOT_DOT:
            return "[TOKEN_DOT_DOT]";
            break;
        case TOKEN_THREE_DOTS:
            return "[TOKEN_THREE_DOTS]";
            break;
        case TOKEN_AND:
            return "[TOKEN_AND]";
            break;
        case TOKEN_BREAK:
            return "[TOKEN_BREAK]";
            break;
        case TOKEN_DO:
            return "[TOKEN_DO]";
            break;
        case TOKEN_ELSE:
            return "[TOKEN_ELSE]";
            break;
        case TOKEN_ELSEIF:
            return "[TOKEN_ELSEIF]";
            break;
        case TOKEN_END:
            return "[TOKEN_END]";
            break;
        case TOKEN_FALSE:
            return "[TOKEN_FALSE]";
            break;
        case TOKEN_FOR:
            return "[TOKEN_FOR]";
            break;
        case TOKEN_FUNCTION:
            return "[TOKEN_FUNCTION]";
            break;
        case TOKEN_IF:
            return "[TOKEN_IF]";
            break;
        case TOKEN_IN:
            return "[TOKEN_IN]";
            break;
        case TOKEN_LOCAL:
            return "[TOKEN_LOCAL]";
            break;
        case TOKEN_NIL:
            return "[TOKEN_NIL]";
            break;
        case TOKEN_NOT:
            return "[TOKEN_NOT]";
            break;
        case TOKEN_OR:
            return "[TOKEN_OR]";
            break;
        case TOKEN_REPEAT:
            return "[TOKEN_REPEAT]";
            break;
        case TOKEN_RETURN:
            return "[TOKEN_RETURN]";
            break;
        case TOKEN_THEN:
            return "[TOKEN_THEN]";
            break;
        case TOKEN_TRUE:
            return "[TOKEN_TRUE]";
            break;
        case TOKEN_UNTIL:
            return "[TOKEN_UNTIL]";
            break;
        case TOKEN_WHILE:
            return "[TOKEN_WHILE]";
            break;
        case TOKEN_IDENTIFIER:
            return "[TOKEN_IDENTIFIER]";
            break;
        case TOKEN_STRING:
            return "[TOKEN_STRING]";
            break;
        case TOKEN_NUMBER:
            return "[TOKEN_NUMBER]";
            break;
        case TOKEN_EOF:
            return "[TOKEN_EOF]";
            break;
        case TOKEN_ERROR:
            break;
    }
}
#endif
