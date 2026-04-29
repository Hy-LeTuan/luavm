#include <lexer.h>
#include <token.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>

void initLexer(const char* source, Lexer* lexer)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

static bool isAlpha(char c)
{
    return isalpha(c) || c == '_';
}

static bool isAlphaNum(char c)
{
    return isalnum(c) || c == '_';
}

static bool isDigit(char c)
{
    return isdigit(c);
}

static Token makeToken(TokenType type, Lexer* lexer)
{
    Token token;

    token.type = type;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;

    return token;
}

static Token error(const char* message, Lexer* lexer)
{
    fprintf(
      stderr, "Error when lexing at line %zu, character '%c'.\n", lexer->line, *lexer->current);
    fprintf(stderr, "Error reads: %s.", message);

    return makeToken(TOKEN_ERROR, lexer);
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

static char prev(Lexer* lexer)
{
    if (isAtEnd(lexer))
    {
        return '\0';
    }

    return *(lexer->current - 1);
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

static Token identifier(Lexer* lexer)
{
    while (!isAtEnd(lexer) && isAlphaNum(peek(lexer)))
    {
        advance(lexer);
    }

    return makeToken(TOKEN_IDENTIFIER, lexer);
}

static Token keyword(TokenType type, const char* word, int length, Lexer* lexer)
{
    if (strncmp(lexer->start, word, length) == 0)
    {
        // if the keyword actually ends and not a part of a bigger identifer
        if (!isAlpha(*(lexer->start + length)))
        {
            lexer->current = lexer->start + length;
            return makeToken(type, lexer);
        }
    }

    return identifier(lexer);
}

static Token number(Lexer* lexer)
{
    while (!isAtEnd(lexer) && isDigit(peek(lexer)))
    {
        advance(lexer);
    }

    // parse the decimal point
    if (peek(lexer) == '.')
    {
        advance(lexer);
        const char* start = lexer->current;

        while (!isAtEnd(lexer) && isDigit(peek(lexer)))
        {
            advance(lexer);
        }

        if (start == lexer->current)
        {
            return error("Error, the decimal part of a number cannot be empty.", lexer);
        }
    }

    return makeToken(TOKEN_NUMBER, lexer);
}

static Token string(char close, Lexer* lexer)
{
    while (!isAtEnd(lexer) && peek(lexer) != close)
    {
        advance(lexer);
    }

    if (isAtEnd(lexer))
    {
        return error("Error, string literal is not properly closed.", lexer);
    }

    // get the closing quote
    advance(lexer);

    return makeToken(TOKEN_STRING, lexer);
}

Token lex(Lexer* lexer)
{
    lexer->start = lexer->current;

    char c = advance(lexer);

    switch (c)
    {
        // symbols
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

                    // skip the 2 double braces
                    for (int i = 0; i < 2; i++)
                    {
                        if (peek(lexer) != ']')
                        {
                            return error("Multiline comment is not properly closed", lexer);
                        }
                        advance(lexer);
                    }
                }
                else
                {
                    while (peek(lexer) != '\n')
                    {
                        advance(lexer);
                    }
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
        // keywords
        case 'a':
            return keyword(TOKEN_AND, "and", 3, lexer);
        case 'b':
            return keyword(TOKEN_BREAK, "break", 5, lexer);
        case 'd':
            return keyword(TOKEN_DO, "do", 2, lexer);
        case 'e':
            if (match('n', lexer))
            {
                return keyword(TOKEN_END, "end", 3, lexer);
            }
            else if (match('l', lexer) && match('s', lexer) && match('e', lexer))
            {
                if (match('i', lexer))
                {
                    return keyword(TOKEN_ELSEIF, "elseif", 6, lexer);
                }
                else
                {
                    return keyword(TOKEN_ELSE, "else", 4, lexer);
                }
            }
            else
            {
                return identifier(lexer);
            }
        case 'f':
            if (peek(lexer) == 'a')
            {
                return keyword(TOKEN_FALSE, "false", 5, lexer);
            }
            else if (peek(lexer) == 'o')
            {
                return keyword(TOKEN_FOR, "for", 3, lexer);
            }
            else
            {
                return keyword(TOKEN_FUNCTION, "function", 8, lexer);
            }
        case 'i':
            if (match('f', lexer))
            {
                return keyword(TOKEN_IF, "if", 2, lexer);
            }
            else
            {
                return keyword(TOKEN_IN, "in", 2, lexer);
            }
        case 'l':
            return keyword(TOKEN_LOCAL, "local", 5, lexer);
        case 'n':
            if (match('i', lexer))
            {
                return keyword(TOKEN_NIL, "nil", 3, lexer);
            }
            else
            {
                return keyword(TOKEN_NOT, "not", 3, lexer);
            }
        case 'o':
            return keyword(TOKEN_OR, "or", 2, lexer);
        case 'r':
            if (match('e', lexer))
            {
                if (match('p', lexer))
                {
                    return keyword(TOKEN_REPEAT, "repeat", 6, lexer);
                }
                else
                {
                    return keyword(TOKEN_RETURN, "return", 6, lexer);
                }
            }
            else
            {
                return identifier(lexer);
            }
        case 't':
            if (match('h', lexer))
            {
                return keyword(TOKEN_THEN, "then", 4, lexer);
            }
            else
            {
                return keyword(TOKEN_TRUE, "true", 4, lexer);
            }
        case 'u':
            return keyword(TOKEN_UNTIL, "until", 5, lexer);
        case 'w':
            return keyword(TOKEN_WHILE, "while", 5, lexer);
        case ' ':
        case '\t':
            skipWhitespace(lexer);
            return lex(lexer);
        case '\n':
            lexer->line++;
            return lex(lexer);
        case '\0':
            return makeToken(TOKEN_EOF, lexer);
        case '\'':
            return string('\'', lexer);
        case '\"':
            return string('\"', lexer);
        default:
            if (isAlpha(c))
            {
                return identifier(lexer);
            }
            else if (isdigit(c))
            {
                return number(lexer);
            }

            return makeToken(TOKEN_ERROR, lexer);
    }
}

#ifdef DEBUG_PRINT_TOKEN
const char* convertToken(Token* token)
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
            return "[TOKEN_ERROR]";
        default:
            return "";
    }
}
#endif
