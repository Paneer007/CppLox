#ifndef clox_scanner_h
#define clox_scanner_h

/**
 * @brief Enumeration representing token types used by the scanner.
 *
 * This enumeration defines the different types of tokens that can be recognized
 * by the scanner. These tokens are used to represent various language
 * constructs and literals.
 */
typedef enum
{
  // Single-character tokens.
  TOKEN_LEFT_PAREN,  ///< Left parenthesis '('
  TOKEN_RIGHT_PAREN,  ///< Right parenthesis ')'
  TOKEN_LEFT_BRACE,  ///< Left brace '{'
  TOKEN_RIGHT_BRACE,  ///< Right brace '}'
  TOKEN_COMMA,  ///< Comma ','
  TOKEN_DOT,  ///< Dot '.'
  TOKEN_MINUS,  ///< Minus '-'
  TOKEN_PLUS,  ///< Plus '+'
  TOKEN_SEMICOLON,  ///< Semicolon ';'
  TOKEN_SLASH,  ///< Slash '/'
  TOKEN_STAR,  ///< Star '*'
  TOKEN_MODULUS,  ///< Modulus '%'

  // One or two character tokens.
  TOKEN_BANG,  ///< Bang '!'
  TOKEN_BANG_EQUAL,  ///< Bang equal '!='
  TOKEN_EQUAL,  ///< Equal '='
  TOKEN_EQUAL_EQUAL,  ///< Equal equal '=='
  TOKEN_GREATER,  ///< Greater than '>'
  TOKEN_GREATER_EQUAL,  ///< Greater than or equal to '>='
  TOKEN_LESS,  ///< Less than '<'
  TOKEN_LESS_EQUAL,  ///< Less than or equal to '<='

  // Literals.
  TOKEN_IDENTIFIER,  ///< Identifier
  TOKEN_STRING,  ///< String literal
  TOKEN_NUMBER,  ///< Number literal

  // Keywords.
  TOKEN_AND,  ///< And keyword 'and'
  TOKEN_CLASS,  ///< Class keyword 'class'
  TOKEN_ELSE,  ///< Else keyword 'else'
  TOKEN_FALSE,  ///< False keyword 'false'
  TOKEN_FOR,  ///< For keyword 'for'
  TOKEN_FUN,  ///< Function keyword 'fun'
  TOKEN_IF,  ///< If keyword 'if'
  TOKEN_NIL,  ///< Nil keyword 'nil'
  TOKEN_OR,  ///< Or keyword 'or'
  TOKEN_PRINT,  ///< Print keyword 'print'
  TOKEN_RETURN,  ///< Return keyword 'return'
  TOKEN_SUPER,  ///< Super keyword 'super'
  TOKEN_THIS,  ///< This keyword 'this'
  TOKEN_TRUE,  ///< True keyword 'true'
  TOKEN_VAR,  ///< Var keyword 'var'
  TOKEN_WHILE,  ///< While keyword 'while'

  TOKEN_ERROR,  ///< Error token
  TOKEN_EOF,  ///< End of file

  TOKEN_LEFT_BRACKET,
  TOKEN_RIGHT_BRACKET
} TokenType;

/**
 * @brief Represents a token in the source code.
 *
 * A token encapsulates information about a lexical unit in the input stream,
 * including its type, starting position, length, and line number.
 */
class Token
{
public:
  TokenType type;
  const char* start;
  int length;
  int line;
};

/**
 * @brief Represents a lexical scanner for parsing source code.
 *
 * This class is responsible for tokenizing the input source code. It maintains
 * the current scanning position, handles whitespace, comments, and various
 * token types.
 */
class Scanner
{
  /**
   * @brief Default constructor (private to prevent direct instantiation).
   */
  Scanner() {}

public:
  /**
   * @brief The singleton scanner instance.
   */
  static Scanner* scanner;

  /**
   * @brief Pointer to the beginning of the current token.
   */
  const char* start;

  /**
   * @brief Pointer to the current character in the source code.
   */
  const char* current;

  /**
   * @brief Current line number.
   */
  int line;

  /**
   * @brief Initializes the scanner with the given source code.
   *
   * @param source The source code to be scanned.
   */
  void initScanner(const char* source);

  /**
   * Scans the next token from the input stream.
   *
   * Advances the input pointer and identifies the next token based on the
   * character encountered. Handles single-character tokens, numbers,
   * identifiers, strings, and two-character tokens.
   *
   * @return The next token in the input stream.
   */
  Token scanToken();

  /**
   * @brief Gets the singleton scanner instance.
   *
   * @return A pointer to the scanner instance.
   */
  static Scanner* getScanner();

  /**
   * @brief Checks if the scanner has reached the end of the input.
   *
   * Determines whether the current character pointer has reached the null
   * terminator of the source code.
   *
   * @return `true` if the end of the input has been reached, `false` otherwise.
   */
  bool isAtEnd();

  /**
   * @brief Creates a new token with the given type.
   *
   * Constructs a token object with the specified type, starting position,
   * length, and line number.
   *
   * @param type The type of the token to be created.
   * @return The newly created token.
   */
  Token makeToken(TokenType type);

  /**
   * @brief Creates an error token with the given message.
   *
   * Constructs a token object indicating an error with the specified error
   * message.
   *
   * @param message The error message to be included in the token.
   * @return The error token.
   */
  Token errorToken(const char* message);

  /**
   * @brief Advances the scanner to the next character.
   *
   * Increments the current character pointer and returns the previous
   * character.
   *
   * @return The character that was at the previous position.
   */
  char advance();

  /**
   * @brief Checks if the next character matches the expected character.
   *
   * Consumes the next character if it matches the expected character.
   *
   * @param expected The expected character.
   * @return `true` if the next character matches, `false` otherwise.
   */
  bool match(char expected);

  /**
   * @brief Skips whitespace and comments.
   *
   * Consumes whitespace characters (spaces, tabs, newlines, and carriage
   * returns) and single-line comments. Increments the line number when
   * encountering newlines.
   */
  void skipWhitespace();

  /**
   * @brief Returns the current character without advancing the scanner.
   *
   * @return The current character.
   */
  char peek();

  /**
   * @brief Peeks at the next character without advancing the scanner.
   *
   * Returns the next character in the input stream without moving the current
   * character pointer. If the end of the input has been reached, returns the
   * null character.
   *
   * @return The next character in the input stream.
   */
  char peekNext();

  /**
   * @brief Scans a string literal.
   *
   * Reads characters until a closing double quote is encountered.
   * Handles newlines within the string by incrementing the line count.
   * Reports an error if the string is unterminated.
   *
   * @return The string token.
   */
  Token string();

  /**
   * @brief Checks if a character is a digit.
   *
   * Determines if the given character is a numeric digit between 0 and 9.
   *
   * @param c The character to check.
   * @return `true` if the character is a digit, `false` otherwise.
   */
  bool isDigit(char c);

  /**
   * @brief Scans a number literal.
   *
   * Reads digits until a non-digit character is encountered.
   * Handles fractional parts by consuming digits after a decimal point.
   *
   * @return The number token.
   */
  Token number();

  /**
   * @brief Checks if a character is an alphabetic character or an underscore.
   *
   * Determines if the given character is considered an alphabetic character or
   * an underscore. Alphabetic characters include both uppercase and lowercase
   * letters.
   *
   * @param c The character to check.
   * @return `true` if the character is alphabetic or an underscore, `false`
   * otherwise.
   */
  bool isAlpha(char c);

  /**
   * Scans an identifier token.
   *
   * Consumes characters as long as they are alphanumeric or underscores.
   * Once the identifier is complete, it creates a token of the appropriate type
   * based on keywords.
   *
   * @return The token representing the scanned identifier.
   */
  Token identifier();

  /**
   * Determines the token type of an identifier.
   *
   * Checks if the identifier matches any of the predefined keywords.
   * If a match is found, returns the corresponding token type.
   * Otherwise, returns TOKEN_IDENTIFIER.
   *
   * @return The token type of the identifier.
   */
  TokenType identifierType();

  /**
   * Checks if the current token is a keyword.
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
  TokenType checkKeyword(int start,
                         int length,
                         const char* rest,
                         TokenType type);
};
#endif
