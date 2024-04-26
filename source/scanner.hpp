#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum
{
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR,
  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND,
  TOKEN_CLASS,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FUN,
  TOKEN_IF,
  TOKEN_NIL,
  TOKEN_OR,
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_TRUE,
  TOKEN_VAR,
  TOKEN_WHILE,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

class Token
{
public:
  TokenType type;
  const char* start;
  int length;
  int line;
};

class Scanner
{
  Scanner() {}

public:
  static Scanner* scanner;
  const char* start;
  const char* current;
  int line;

  void initScanner(const char* source);
  Token scanToken();
  static Scanner* getScanner();
  bool isAtEnd();

  Token makeToken(TokenType type);
  Token errorToken(const char* message);

  char advance();
  bool match(char expected);
  void skipWhitespace();
  char peek();
  char peekNext();
  Token string();
  bool isDigit(char c);
  Token number();

  bool isAlpha(char c);
  Token identifier();
  TokenType identifierType();
  TokenType checkKeyword(int start,
                         int length,
                         const char* rest,
                         TokenType type);
};
#endif
