#include "scanner.hpp"

#include <stdio.h>
#include <string.h>

#include "common.hpp"

void Scanner::initScanner(const char* source)
{
  this->start = source;
  this->current = source;
  this->line = 1;
}

Scanner* Scanner::getScanner()
{
  return Scanner::scanner;
}

bool Scanner::isAlpha(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Token Scanner::identifier()
{
  while (this->isAlpha(peek()) || this->isDigit(peek()))
    this->advance();
  return this->makeToken(this->identifierType());
}

TokenType Scanner::checkKeyword(int start,
                                int length,
                                const char* rest,
                                TokenType type)
{
  if (this->current - this->start == start + length
      && memcmp(this->start + start, rest, length) == 0)
  {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

TokenType Scanner::identifierType()
{
  switch (this->start[0]) {
    case 'a':
      return this->checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
      return this->checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
      return this->checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (this->current - this->start > 1) {
        switch (this->start[1]) {
          case 'a':
            return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'o':
            return checkKeyword(2, 1, "r", TOKEN_FOR);
          case 'u':
            return checkKeyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'i':
      return this->checkKeyword(1, 1, "f", TOKEN_IF);

    case 'n':
      return this->checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
      return this->checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
      return this->checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
      return this->checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
      return this->checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (this->current - this->start > 1) {
        switch (this->start[1]) {
          case 'h':
            return checkKeyword(2, 2, "is", TOKEN_THIS);
          case 'r':
            return checkKeyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'v':
      return this->checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
      return this->checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

bool Scanner::isAtEnd()
{
  return *(this->current) == '\0';
}

char Scanner::advance()
{
  this->current++;
  return this->current[-1];
}

char Scanner::peekNext()
{
  if (isAtEnd())
    return '\0';
  return this->current[1];
}

Token Scanner::string()
{
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n')
      this->line++;
    advance();
  }

  if (isAtEnd())
    return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}

bool Scanner::match(char expected)
{
  if (isAtEnd())
    return false;
  if (*this->current != expected)
    return false;
  this->current++;
  return true;
}

char Scanner::peek()
{
  return *this->current;
}

bool Scanner::isDigit(char c)
{
  return c >= '0' && c <= '9';
}

Token Scanner::number()
{
  while (this->isDigit(peek()))
    this->advance();

  // Look for a fractional part.
  if (this->peek() == '.' && this->isDigit(this->peekNext())) {
    // Consume the ".".
    this->advance();

    while (this->isDigit(peek()))
      this->advance();
  }

  return this->makeToken(TOKEN_NUMBER);
}

void Scanner::skipWhitespace()
{
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        this->advance();
        break;
      case '\n':
        this->line++;
        this->advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd())
            advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

Token Scanner::scanToken()
{
  this->skipWhitespace();
  this->start = this->current;

  if (isAtEnd())
    return this->makeToken(TOKEN_EOF);

  char c = this->advance();

  if (this->isDigit(c))
    return this->number();

  if (isAlpha(c))
    return identifier();

  switch (c) {
    case '(':
      return makeToken(TOKEN_LEFT_PAREN);
    case ')':
      return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
      return makeToken(TOKEN_LEFT_BRACE);
    case '}':
      return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
      return makeToken(TOKEN_SEMICOLON);
    case ',':
      return makeToken(TOKEN_COMMA);
    case '.':
      return makeToken(TOKEN_DOT);
    case '-':
      return makeToken(TOKEN_MINUS);
    case '+':
      return makeToken(TOKEN_PLUS);
    case '/':
      return makeToken(TOKEN_SLASH);
    case '*':
      return makeToken(TOKEN_STAR);
    case '!':
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"':
      return string();
  }

  return this->errorToken("Unexpected character.");
}

Token Scanner::makeToken(TokenType type)
{
  Token token;
  token.type = type;
  token.start = this->start;
  token.length = (int)(this->current - this->start);
  token.line = this->line;
  return token;
}

Token Scanner::errorToken(const char* message)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = this->line;
  return token;
}

Scanner* Scanner::scanner = new Scanner;