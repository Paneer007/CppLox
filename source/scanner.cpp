#include "scanner.hpp"

#include <stdio.h>
#include <string.h>

#include "common.hpp"

/**
 * @brief Initializes the scanner with the given source code.
 *
 * This function sets up the scanner by storing the beginning and current
 * positions of the source code, as well as the initial line number.
 *
 * @param source The source code to be scanned.
 */
void Scanner::initScanner(const char* source)
{
  this->start = source;
  this->current = source;
  this->line = 1;
}

/**
 * @brief Retrieves the singleton scanner instance.
 *
 * This static method returns a reference to the globally accessible scanner
 * object.
 *
 * @return A pointer to the scanner instance.
 */
Scanner* Scanner::getScanner()
{
  return Scanner::scanner;
}

/**
 * @brief Checks if a character is an alphabetic character or an underscore.
 *
 * Determines if the given character is considered an alphabetic character or an
 * underscore. Alphabetic characters include both uppercase and lowercase
 * letters.
 *
 * @param c The character to check.
 * @return `true` if the character is alphabetic or an underscore, `false`
 * otherwise.
 */
bool Scanner::isAlpha(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/**
 * @brief Scans an identifier token.
 *
 * This function consumes characters as long as they are alphanumeric or
 * underscores. Once the identifier is complete, it creates a token of the
 * appropriate type based on keywords.
 *
 * @return The token representing the scanned identifier.
 */
Token Scanner::identifier()
{
  while (this->isAlpha(peek()) || this->isDigit(peek()))
    this->advance();
  return this->makeToken(this->identifierType());
}

/**
 * @brief Checks if the current token is a keyword.
 *
 * Compares the current token with a given keyword and its length.
 * If they match, returns the corresponding token type.
 * Otherwise, returns TOKEN_IDENTIFIER.
 *
 * @param start The starting index of the potential keyword.
 * @param length The length of the potential keyword.
 * @param rest The remaining characters of the keyword.
 * @param type The token type if the keyword matches.
 * @return The token type, either the specified type for a match or
 * TOKEN_IDENTIFIER.
 */
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

/**
 * @brief Determines the token type of an identifier.
 *
 * Checks if the identifier matches any of the predefined keywords.
 * If a match is found, returns the corresponding token type.
 * Otherwise, returns TOKEN_IDENTIFIER.
 *
 * @return The token type of the identifier.
 */
TokenType Scanner::identifierType()
{
  switch (this->start[0]) {
    case 'a':
      if (this->current - this->start > 1) {
        switch (this->start[1]) {
          case 'w':
            return this->checkKeyword(2, 3, "ait", TOKEN_AWAIT);
          case 's':
            return this->checkKeyword(2, 3, "ync", TOKEN_ASYNC);
          case 'n':
            return this->checkKeyword(2, 1, "d", TOKEN_AND);
        }
      }
      break;
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
            if (this->current - this->start > 2) {
              switch (this->start[2]) {
                case 'n':
                  return checkKeyword(3, 0, "", TOKEN_FUN);
                case 't':
                  return checkKeyword(3, 3, "ure", TOKEN_FUTURE);
              }
            }
          case 'i':
            return checkKeyword(2, 4, "nish", TOKEN_FINISH);
        }
      }
      break;
    case 'i':
      return this->checkKeyword(1, 1, "f", TOKEN_IF);
    case 'l':
      return this->checkKeyword(1, 5, "ambda", TOKEN_LAMBDA);
    case 'n':
      return this->checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
      return this->checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
      return this->checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
      if (this->current - this->start > 1 && this->start[1] == 'e'
          && this->current - this->start > 2)
      {
        switch (this->start[2]) {
          case 'd':

            return this->checkKeyword(3, 3, "uce", TOKEN_REDUCE);
          case 't':
            return this->checkKeyword(3, 3, "urn", TOKEN_RETURN);
        }
      }
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

/**
 * @brief  Checks if the scanner has reached the end of the input.
 *
 * Determines whether the current character pointer has reached the null
 * terminator of the source code.
 *
 * @return `true` if the end of the input has been reached, `false` otherwise.
 */
bool Scanner::isAtEnd()
{
  return *(this->current) == '\0';
}

/**
 * @brief Advances the scanner to the next character.
 *
 * Increments the current character pointer and returns the previous
 * character.
 *
 * @return The character that was at the previous position.
 */
char Scanner::advance()
{
  this->current++;
  return this->current[-1];
}

/**
 * @brief Peeks at the next character without advancing the scanner.
 *
 * Returns the next character in the input stream without moving the current
 * character pointer. If the end of the input has been reached, returns the
 * null character.
 *
 * @return The next character in the input stream.
 */
char Scanner::peekNext()
{
  if (isAtEnd())
    return '\0';
  return this->current[1];
}

/**
 * @brief Scans a string literal.
 *
 * Reads characters until a closing double quote is encountered.
 * Handles newlines within the string by incrementing the line count.
 * Reports an error if the string is unterminated.
 *
 * @return The string token.
 */
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

/**
 * @brief Checks if the next character matches the expected character.
 *
 * Consumes the next character if it matches the expected character.
 *
 * @param expected The expected character.
 * @return `true` if the next character matches, `false` otherwise.
 */
bool Scanner::match(char expected)
{
  if (isAtEnd())
    return false;
  if (*this->current != expected)
    return false;
  this->current++;
  return true;
}

/**
 * @brief Returns the current character without advancing the scanner.
 *
 * @return The current character.
 */
char Scanner::peek()
{
  return *this->current;
}

/**
 * @brief Checks if a character is a digit.
 *
 * Determines if the given character is a numeric digit between 0 and 9.
 *
 * @param c The character to check.
 * @return `true` if the character is a digit, `false` otherwise.
 */
bool Scanner::isDigit(char c)
{
  return c >= '0' && c <= '9';
}

/**
 * @brief Scans a number literal.
 *
 * Reads digits until a non-digit character is encountered.
 * Handles fractional parts by consuming digits after a decimal point.
 *
 * @return The number token.
 */
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

/**
 * @brief Skips whitespace and comments.
 *
 * Consumes whitespace characters (spaces, tabs, newlines, and carriage
 * returns) and single-line comments. Increments the line number when
 * encountering newlines.
 *
 */
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

/**
 * @brief Scans the next token from the input stream.
 *
 * Advances the input pointer and identifies the next token based on the
 * character encountered. Handles single-character tokens, numbers,
 * identifiers, strings, and two-character tokens.
 *
 * @return The next token in the input stream.
 */
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
    case ':':
      return makeToken(TOKEN_COLON);
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
    case '%':
      return makeToken(TOKEN_MODULUS);
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
    case '[':
      return makeToken(TOKEN_LEFT_BRACKET);
    case ']':
      return makeToken(TOKEN_RIGHT_BRACKET);
  }

  return this->errorToken("Unexpected character.");
}

/**
 * @brief Creates a new token with the given type.
 *
 * Constructs a token object with the specified type, starting position,
 * length, and line number.
 *
 * @param type The type of the token to be created.
 * @return The newly created token.
 */
Token Scanner::makeToken(TokenType type)
{
  Token token;
  token.type = type;
  token.start = this->start;
  token.length = (int)(this->current - this->start);
  token.line = this->line;
  return token;
}

/**
 * @brief Creates an error token with the given message.
 *
 * Constructs a token object indicating an error with the specified error
 * message.
 *
 * @param message The error message to be included in the token.
 * @return The error token.
 */
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