#include "compiler.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "scanner.hpp"

#ifdef DEBUG_PRINT_CODE
#  include "debug.hpp"
#endif

/**
 * @brief Represents the parser's state during parsing.
 *
 * Stores information about the current token, previous token,
 * error status, and panic mode recovery state.
 *
 * @details
 *  - `current`: The current token being processed.
 *  - `previous`: The previous token processed.
 *  - `hadError`: Flag indicating if a syntax error has occurred.
 *  - `panicMode`: Flag indicating if the parser is in panic mode (recovering
 * from errors).
 */
class Parser
{
public:
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
};

/**
 * @brief Represents operator precedence levels.
 *
 * Lower numerical values indicate higher precedence.
 */
typedef enum
{
  PREC_NONE,  ///< No precedence
  PREC_ASSIGNMENT,  ///< Assignment (=)
  PREC_OR,  ///< Logical OR (or)
  PREC_AND,  ///< Logical AND (and)
  PREC_EQUALITY,  ///< Equality (==, !=)
  PREC_COMPARISON,  ///< Comparison (<, >, <=, >=)
  PREC_TERM,  ///< Term (+, -)
  PREC_FACTOR,  ///< Factor (*, /)
  PREC_UNARY,  ///< Unary (!, -)
  PREC_CALL,  ///< Function call (.)
  PREC_SUBSCRIPT,
  PREC_PRIMARY  ///< Primary (highest precedence)
} Precedence;

/**
 * @brief Typedef for a function pointer used in parsing.
 *
 * The function takes a boolean argument indicating whether
 * an assignment is allowed.
 */
typedef void (*ParseFn)(bool canAssign);

/**
 * @brief Encapsulates parsing rules for operators.
 *
 * Stores function pointers for prefix and infix parsing, along with operator
 * precedence.
 *
 * @details
 *  - `prefix`: Function pointer for prefix parsing.
 *  - `infix`: Function pointer for infix parsing.
 *  - `precedence`: Precedence level of the operator.
 */
class ParseRule
{
public:
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};

/**
 * @brief Represents a local variable.
 *
 * Stores information about a local variable's name, depth in the scope chain,
 * and capture status.
 *
 * @details
 *  - `name`: The name of the local variable.
 *  - `depth`: The depth of the variable in the scope chain.
 *  - `isCaptured`: Flag indicating if the variable is captured in a closure.
 */
class Local
{
public:
  Token name;
  int depth;
  bool isCaptured;
};

/**
 * @brief Represents an upvalue (closed-over variable).
 *
 * Stores the index of the closed-over variable and its origin (local or
 * global).
 *
 * @details
 *  - `index`: Index of the closed-over variable.
 *  - `isLocal`: Flag indicating whether the upvalue is a local variable.
 */
class Upvalue
{
public:
  uint8_t index;
  bool isLocal;
};

/**
 * @brief Enumeration representing different function types.
 *
 * Defines constants for function, initializer, script, and method types.
 */
typedef enum
{
  TYPE_FUNCTION,  // Regular function
  TYPE_INITIALIZER,  // Initializer function
  TYPE_SCRIPT,  // Script
  TYPE_METHOD,  // Method
  TYPE_FINISH,
  TYPE_ASYNC
} FunctionType;

/**
 * @brief Represents a compiler instance.
 *
 * Stores information about the compilation context, including enclosing
 * compiler, current function, function type, local variables, upvalues, and
 * scope depth.
 *
 * @details
 *  - `enclosing`: The enclosing compiler (for nested functions).
 *  - `function`: The function being compiled.
 *  - `type`: The type of function being compiled.
 *  - `locals`: Array of local variables.
 *  - `localCount`: Count of local variables.
 *  - `upvalues`: Array of upvalues.
 *  - `scopeDepth`: Current scope depth.
 */
class Compiler
{
public:
  Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
};

/**
 * @brief Represents a compiler instance for a class.
 *
 * Stores information about the enclosing class compiler and superclass
 * existence.
 *
 * @details
 *  - `enclosing`: The enclosing class compiler (for nested classes).
 *  - `hasSuperclass`: Flag indicating if the class has a superclass.
 */
class ClassCompiler
{
public:
  ClassCompiler* enclosing;
  bool hasSuperclass;
};

/**
 * @brief Global parser instance.
 */
static Parser parser;

/**
 * @brief Pointer to the current compiler instance.
 */
static Compiler* current = NULL;

/**
 * @brief Pointer to the current class compiler instance.
 */
static ClassCompiler* currentClass = NULL;

/**
 * @brief Parses an expression.
 */
static void expression();

/**
 * @brief Parses a declaration.
 */
static void declaration();

/**
 * @brief Parses a statement.
 */
static void statement();

/**
 * @brief Retrieves the parsing rule for a given token type.
 *
 * @param type The token type.
 * @return A pointer to the corresponding parse rule.
 */
static ParseRule* getRule(TokenType type);

/**
 * @brief Parses an expression with the given precedence.
 *
 * @param precedence The minimum precedence to parse.
 */
static void parsePrecedence(Precedence precedence);

/**
 * @brief Returns a pointer to the current chunk.
 *
 * Retrieves the chunk associated with the current function.
 *
 * @return A pointer to the current chunk.
 */
static Chunk* currentChunk()
{
  return &current->function->chunk;
}

/**
 * @brief Reports an error at the given token location.
 *
 * Prints an error message to stderr, sets panic mode, and marks the parser as
 * having an error.
 *
 * @param token The token where the error occurred.
 * @param message The error message to display.
 */
static void errorAt(Token* token, const char* message)
{
  if (parser.panicMode)
    return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

/**
 * @brief Reports an error at the current token location.
 *
 * A convenience wrapper for `errorAt` that uses the current token.
 *
 * @param message The error message to display.
 */
static void errorAtCurrent(const char* message)
{
  errorAt(&parser.current, message);
}

/**
 * @brief Reports an error at the previous token location.
 *
 * A convenience wrapper for `errorAt` that uses the previous token.
 *
 * @param message The error message to display.
 */
static void error(const char* message)
{
  errorAt(&parser.previous, message);
}

/**
 * @brief Advances the parser to the next token.
 *
 * Consumes the current token, updates the previous token, and scans for the
 * next token. Handles error tokens by reporting an error and continuing to scan
 * until a valid token is found.
 */
static void advance()
{
  parser.previous = parser.current;
  auto scanner = Scanner::getScanner();
  for (;;) {
    parser.current = scanner->scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

/**
 * @brief Creates a constant in the current chunk.
 *
 * Adds the given value as a constant to the current chunk and returns its
 * index. Handles errors if the constant count exceeds the maximum allowed.
 *
 * @param value The value to be added as a constant.
 * @return The index of the constant in the chunk, or 0 on error.
 */
static uint8_t makeConstant(Value value)
{
  auto constant = currentChunk()->addConstant(value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

/**
 * @brief Consumes the current token if it matches the expected type.
 *
 * Advances the parser if the current token matches the given type, otherwise
 * reports an error.
 *
 * @param type The expected token type.
 * @param message The error message to display if the token doesn't match.
 */
static void consume(TokenType type, const char* message)
{
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

/**
 * @brief Checks if the current token matches the given type.
 *
 * Returns true if the current token's type matches the provided type, otherwise
 * false.
 *
 * @param type The token type to check.
 * @return True if the current token matches the type, false otherwise.
 */
static bool check(TokenType type)
{
  return parser.current.type == type;
}

/**
 * @brief Consumes the current token if it matches the given type.
 *
 * Advances the parser if the current token matches the provided type, otherwise
 * returns false.
 *
 * @param type The expected token type.
 * @return True if the current token was consumed, false otherwise.
 */
static bool match(TokenType type)
{
  if (!check(type))
    return false;
  advance();
  return true;
}

/**
 * @brief Writes a byte to the current chunk.
 *
 * Adds the given byte to the chunk's code array and records the line number.
 *
 * @param byte The byte to write.
 */
static void emitByte(uint8_t byte)
{
  currentChunk()->writeChunk(byte, parser.previous.line);
}

/**
 * @brief Writes two bytes to the current chunk.
 *
 * Writes the given bytes sequentially to the chunk's code array, recording the
 * line number for each byte.
 *
 * @param byte1 The first byte to write.
 * @param byte2 The second byte to write.
 */
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
  emitByte(byte1);
  emitByte(byte2);
}

/**
 * @brief Emits a jump instruction and reserves space for the offset.
 *
 * Writes the given instruction byte and two placeholder bytes for the jump
 * offset. Returns the current chunk's count minus 2 to indicate the offset
 * position.
 *
 * @param instruction The jump instruction byte.
 * @return The offset position in the chunk.
 */
static int emitJump(uint8_t instruction)
{
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

/**
 * @brief Emits a return instruction.
 *
 * Emits the appropriate opcode for returning from a function, depending on the
 * function type.
 */
static void emitReturn()
{
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

/**
 * @brief Emits a constant instruction.
 *
 * Creates a constant and emits a constant instruction with its index.
 *
 * @param value The value of the constant.
 */
static void emitConstant(Value value)
{
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/**
 * @brief Patches a jump instruction with the calculated offset.
 *
 * Calculates the jump offset, checks for overflow, and updates the jump
 * instruction bytes.
 *
 * @param offset The position of the jump instruction in the chunk.
 */
static void patchJump(int offset)
{
  // -2 to adjust for the bytecode for the jump offset itself.
  auto jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

/**
 * @brief Initializes a compiler instance.
 *
 * Sets up the compiler's state, including enclosing compiler, function, type,
 * locals, and scope depth. Creates a new function and initializes the first
 * local variable.
 *
 * @param compiler The compiler instance to initialize.
 * @param type The type of function being compiled.
 */
static void initCompiler(Compiler* compiler, FunctionType type)
{
  // Initializes compiler state, creates new function, sets up initial local
  // variable.
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;

  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;

  // Assigns function name if current function is not a script, using
  // information from previous token.
  if (type != TYPE_SCRIPT) {
    current->function->name =
        copyString(parser.previous.start, parser.previous.length);
  }

  // Creates a new local variable, initializes depth and capture status. Sets
  // variable name to "this" for methods, otherwise empty string. Increments
  // local count.
  auto local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

/**
 * @brief Completes the compilation process for the current scope and returns
 * the compiled function.
 *
 * Emits a return instruction, optionally prints the disassembled code for
 * debugging, and returns the compiled function. The current compiler is set to
 * the enclosing compiler.
 *
 * @return The compiled function object.
 */
static ObjFunction* endCompiler()
{
  emitReturn();
  ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(
        currentChunk(),
        function->name != NULL ? function->name->chars : "<script>");
  }
#endif
  current = current->enclosing;
  return function;
}

/**
 * @brief Ends the current scope.
 *
 * Decrements the scope depth and pops local variables from the scope.
 * Handles captured variables by emitting OP_CLOSE_UPVALUE, otherwise emits
 * OP_POP.
 */
static void endScope()
{
  current->scopeDepth--;
  while (current->localCount > 0
         && current->locals[current->localCount - 1].depth
             > current->scopeDepth)
  {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

/**
 * @brief Begins a new scope.
 *
 * Increments the scope depth to indicate the start of a new scope.
 */
static void beginScope()
{
  current->scopeDepth++;
}

/**
 * @brief Creates a constant for an identifier.
 *
 * Creates a string object from the token and adds it as a constant to the
 * chunk.
 *
 * @param name The identifier token.
 * @return The index of the constant in the chunk.
 */
static uint8_t identifierConstant(Token* name)
{
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/**
 * @brief Compares two tokens for identifier equality.
 *
 * Checks if the length and content of the two tokens match.
 *
 * @param a The first token.
 * @param b The second token.
 * @return True if the tokens represent the same identifier, false otherwise.
 */
static bool identifiersEqual(Token* a, Token* b)
{
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

/**
 * @brief Resolves a local variable by its name.
 *
 * Searches the local variable table for a matching identifier.
 * Returns the index of the local variable if found, otherwise returns -1.
 *
 * @param compiler The compiler instance.
 * @param name The identifier to resolve.
 * @return The index of the local variable, or -1 if not found.
 */
static int resolveLocal(Compiler* compiler, Token* name)
{
  for (auto i = compiler->localCount - 1; i >= 0; i--) {
    auto local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

/**
 * @brief Adds an upvalue to the compiler's upvalue list.
 *
 * Checks if the upvalue already exists, otherwise creates a new upvalue and
 * returns its index.
 *
 * @param compiler The compiler instance.
 * @param index The index of the closed-over variable.
 * @param isLocal Indicates whether the upvalue is a local variable.
 * @return The index of the upvalue in the upvalue list.
 */

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal)
{
  // Counts existing upvalues and checks if the given upvalue already exists,
  // returning its index if found.
  int upvalueCount = compiler->function->upvalueCount;
  for (auto i = 0; i < upvalueCount; i++) {
    auto upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  // Checks if the maximum number of upvalues is reached, reports an error and
  // returns 0 if exceeded.
  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

  // Adds a new upvalue to the compiler's upvalue list and returns its index.
  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

/**
 * @brief Resolves an upvalue by searching enclosing scopes.
 *
 * Checks the current scope and its enclosing scopes for the given name.
 * Creates an upvalue if the variable is found in an outer scope.
 *
 * @param compiler The compiler instance.
 * @param name The identifier of the upvalue.
 * @return The index of the upvalue, or -1 if not found.
 */
static int resolveUpvalue(Compiler* compiler, Token* name)
{
  if (compiler->enclosing == NULL)
    return -1;

  // Tries to find local variable in enclosing scope, marks as captured if
  // found, creates upvalue.
  auto local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  // Tries to find upvalue in enclosing scope, adds as upvalue if found.
  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

/**
 * @brief Adds a local variable to the current scope.
 *
 * Creates a new local variable with the given name and initializes its depth
 * and capture status.
 *
 * @param name The name of the local variable.
 */
static void addLocal(Token name)
{
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }
  auto local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

/**
 * @brief Marks the current local variable as initialized.
 *
 * Sets the depth of the current local variable to the current scope depth.
 */
static void markInitialized()
{
  if (current->scopeDepth == 0)
    return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/**
 * @brief Declares a new local variable.
 *
 * Checks for existing variables in the current scope, creates a new local
 * variable, and marks it as uninitialized.
 */
static void declareVariable()
{
  if (current->scopeDepth == 0) {
    return;
  }

  // Checks for existing local variable with same name in current scope, reports
  // error if found.
  auto name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    auto local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

/**
 * @brief Parses a variable declaration.
 *
 * Consumes an identifier token, declares the variable, and creates a constant
 * for it. Returns the index of the constant representing the variable's name.
 *
 * @param errorMessage The error message to display if the token is not an
 * identifier.
 * @return The index of the constant representing the variable's name.
 */
static uint8_t parseVariable(const char* errorMessage)
{
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0)
    return 0;

  return identifierConstant(&parser.previous);
}

/**
 * @brief Defines a global variable.
 *
 * Marks the current local variable as initialized if in a scope, otherwise
 * emits a DEFINE_GLOBAL opcode.
 *
 * @param global The index of the global variable.
 */
static void defineVariable(uint8_t global)
{
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }
  emitBytes(OP_DEFINE_GLOBAL, global);
}

/**
 * @brief Parses a list of arguments.
 *
 * Consumes arguments enclosed in parentheses, with commas separating them.
 * Returns the number of arguments parsed.
 *
 * @return The number of arguments parsed.
 */
static uint8_t argumentList()
{
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}

/**
 * @brief Parses a named variable expression.
 *
 * Resolves the variable, emits either a get or set operation based on canAssign
 * flag, and handles assignments.
 *
 * @param name The name of the variable.
 * @param canAssign Indicates whether assignment is allowed.
 */
static void namedVariable(Token name, bool canAssign)
{
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

/**
 * @brief Parses a variable expression.
 *
 * Handles variable declaration and assignment based on the canAssign flag.
 *
 * @param canAssign Indicates whether assignment is allowed.
 */
static void variable(bool canAssign)
{
  namedVariable(parser.previous, canAssign);
}

/**
 * @brief Creates a synthetic token from a string.
 *
 * Constructs a token object with the given text as its content.
 *
 * @param text The text for the synthetic token.
 * @return The created token.
 */
static Token syntheticToken(const char* text)
{
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

/**
 * @brief Parses a 'super' expression.
 *
 * Handles 'super' keyword usage for method calls and property access.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void super_(bool canAssign)
{
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error("Can't use 'super' in a class with no superclass.");
  }

  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

/**
 * @brief Parses the 'this' keyword.
 *
 * Handles 'this' keyword usage for method calls and property access.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void this_(bool canAssign)
{
  if (currentClass == NULL) {
    error("Can't use 'this' outside of a class.");
    return;
  }
  variable(false);
}

/**
 * @brief Parses a binary expression.
 *
 * Handles binary operators by parsing the right-hand side, emitting the
 * appropriate opcode, and handling precedence.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void binary(bool canAssign)
{
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:
      emitBytes(OP_EQUAL, OP_NOT);
      break;
    case TOKEN_EQUAL_EQUAL:
      emitByte(OP_EQUAL);
      break;
    case TOKEN_GREATER:
      emitByte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emitBytes(OP_LESS, OP_NOT);
      break;
    case TOKEN_LESS:
      emitByte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emitBytes(OP_GREATER, OP_NOT);
      break;
    case TOKEN_PLUS:
      emitByte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emitByte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emitByte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emitByte(OP_DIVIDE);
      break;
    case TOKEN_MODULUS:
      emitByte(OP_MODULUS);
      break;
    default:
      return;  // Unreachable.
  }
}

/**
 * @brief Parses a function call expression.
 *
 * Parses arguments and emits a call opcode with the argument count.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void call(bool canAssign)
{
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

/**
 * @brief Parses a dot operator expression.
 *
 * Handles property access, method calls, and property assignment.
 *
 * @param canAssign Indicates whether assignment is allowed.
 */
static void dot(bool canAssign)
{
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);

  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

/**
 * @brief Parses a literal expression.
 *
 * Emits the appropriate opcode for the given literal type.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void literal(bool canAssign)
{
  switch (parser.previous.type) {
    case TOKEN_FALSE:
      emitByte(OP_FALSE);
      break;
    case TOKEN_NIL:
      emitByte(OP_NIL);
      break;
    case TOKEN_TRUE:
      emitByte(OP_TRUE);
      break;
    default:
      return;  // Unreachable.
  }
}

static void list(bool canAssign)
{
  int itemCount = 0;
  if (!check(TOKEN_RIGHT_BRACKET)) {
    do {
      if (check(TOKEN_RIGHT_BRACKET)) {
        // Trailing comma case
        break;
      }

      parsePrecedence(PREC_OR);

      if (itemCount == UINT8_COUNT) {
        error("Cannot have more than 256 items in a list literal.");
      }
      itemCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after list literal.");

  emitByte(OP_BUILD_LIST);
  emitByte(itemCount);
  return;
}

static void subscript(bool canAssign)
{
  parsePrecedence(PREC_OR);
  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_INDEX_SET);
  } else {
    emitByte(OP_INDEX_GET);
  }
  return;
}

/**
 * @brief Parses a grouped expression.
 *
 * Parses the enclosed expression and consumes the closing parenthesis.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void grouping(bool canAssign)
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/**
 * @brief Parses an expression.
 *
 * Begins parsing with the lowest precedence (assignment).
 */
static void expression()
{
  parsePrecedence(PREC_ASSIGNMENT);
}

/**
 * @brief Emits a loop instruction.
 *
 * Calculates the loop offset and emits the OP_LOOP instruction with the offset.
 *
 * @param loopStart The starting position of the loop.
 */
static void emitLoop(int loopStart)
{
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

/**
 * @brief Parses a block of statements.
 *
 * Parses declarations until the closing brace is encountered.
 */
static void block()
{
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/**
 * @brief Compiles a function declaration.
 *
 * Parses function parameters, body, and creates a function object.
 *
 * @param type The type of function being compiled.
 */
static void function(FunctionType type)
{
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

/**
 * @brief Parses a method declaration.
 *
 * Handles method and initializer declarations, including name resolution and
 * function compilation.
 */
static void method()
{
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);
  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 4
      && memcmp(parser.previous.start, "init", 4) == 0)
  {
    type = TYPE_INITIALIZER;
  }
  function(type);
  emitBytes(OP_METHOD, constant);
}

/**
 * @brief Parses a class declaration.
 *
 * Handles class declaration, inheritance, and class body parsing.
 */
static void classDeclaration()
{
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);

    if (identifiersEqual(&className, &parser.previous)) {
      error("A class can't inherit from itself.");
    }

    beginScope();
    addLocal(syntheticToken("super"));
    defineVariable(0);

    namedVariable(className, false);
    emitByte(OP_INHERIT);
    classCompiler.hasSuperclass = true;
  }

  namedVariable(className, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  emitByte(OP_POP);

  if (classCompiler.hasSuperclass) {
    endScope();
  }

  currentClass = currentClass->enclosing;
}

/**
 * @brief Parses a function declaration.
 *
 * Declares a function and defines it as a global variable.
 */
static void funDeclaration()
{
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void finishDeclaration()
{
  emitByte(OP_FINISH_BEGIN);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  beginScope();
  block();
  endScope();
  emitByte(OP_FINISH_END);
}

static void asyncDeclaration()
{
  auto asyncStart = currentChunk()->count;
  auto exitJump = emitJump(OP_ASYNC_BEGIN);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  beginScope();
  block();
  endScope();
  emitByte(OP_ASYNC_END);
  patchJump(exitJump);
}

/**
 * @brief Parses a variable declaration.
 *
 * Handles variable declaration with optional initializer and defines the
 * variable.
 */
static void varDeclaration()
{
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

/**
 * @brief Parses an expression statement.
 *
 * Parses an expression and emits a POP opcode to discard the result.
 */
static void expressionStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

/**
 * @brief Parses a for statement.
 *
 * Handles the syntax and logic for for loops, including initializer, condition,
 * increment, and body.
 */
static void forStatement()
{
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;

  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);  // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP);  // Condition.
  }

  endScope();
}

/**
 * @brief Parses an if statement.
 *
 * Handles the if and optional else branches.
 */
static void ifStatement()
{
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  auto thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  auto elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);
  if (match(TOKEN_ELSE))
    statement();
  patchJump(elseJump);
}

/**
 * @brief Parses a print statement.
 *
 * Evaluates an expression and emits a print opcode.
 */
static void printStatement()
{
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

/**
 * @brief Parses a return statement.
 *
 * Handles return statements with and without return values, and checks for
 * valid return contexts.
 */
static void returnStatement()
{
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

/**
 * @brief Parses a while statement.
 *
 * Handles the while loop condition, body, and loop control.
 */
static void whileStatement()
{
  auto loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  auto exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

/**
 * @brief Synchronizes the parser after an error.
 *
 * Skips tokens until it reaches the beginning of the next statement or the end
 * of the file.
 */
static void synchronize()
{
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:;  // Do nothing.
    }

    advance();
  }
}

/**
 * @brief Parses a declaration or statement.
 *
 * Handles different declaration and statement types, and performs error
 * recovery if necessary.
 */
static void declaration()
{
  if (match(TOKEN_CLASS)) {
    classDeclaration();
  } else if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }
  if (parser.panicMode)
    synchronize();
}

/**
 * @brief Parses a statement.
 *
 * Handles different types of statements, including expressions, declarations,
 * and control flow.
 */
static void statement()
{
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_FINISH)) {
    finishDeclaration();
  } else if (match(TOKEN_ASYNC)) {
    asyncDeclaration();
  } else {
    expressionStatement();
  }
}

/**
 * @brief Parses a number literal.
 *
 * Converts the number token to a double and emits a constant instruction.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void number(bool canAssign)
{
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/**
 * @brief Parses a string literal.
 *
 * Creates a string object and emits a constant instruction.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void string(bool canAssign)
{
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

/**
 * @brief Parses an expression with the given precedence.
 *
 * Handles operator precedence and associativity.
 *
 * @param precedence The minimum precedence to parse.
 */
static void parsePrecedence(Precedence precedence)
{
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

/**
 * @brief Parses a unary expression.
 *
 * Handles unary operators (negation and logical NOT) by parsing the operand and
 * emitting the corresponding opcode.
 *
 * @param canAssign Indicates whether assignment is allowed (unused in this
 * function).
 */
static void unary(bool canAssign)
{
  auto operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG:
      emitByte(OP_NOT);
      break;
    case TOKEN_MINUS:
      emitByte(OP_NEGATE);
      break;
    default:
      return;  // Unreachable.
  }
}

static void or_(bool canAssign)
{
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void and_(bool canAssign)
{
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

/**
 * @brief Lookup table for parsing rules.
 *
 * Maps token types to prefix and infix parsing functions, along with operator
 * precedence.
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_MODULUS] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {super_, NULL, PREC_NONE},
    [TOKEN_THIS] = {this_, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACKET] = {list, subscript, PREC_SUBSCRIPT},
    [TOKEN_RIGHT_BRACKET] = {NULL, NULL, PREC_NONE},
};

/**
 * @brief Retrieves the parsing rule for a given token type.
 *
 * Returns a pointer to the `ParseRule` for the specified token type.
 *
 * @param type The token type to look up.
 * @return A pointer to the corresponding `ParseRule`.
 */
static ParseRule* getRule(TokenType type)
{
  return &rules[type];
}

/**
 * @brief Compiles the given source code into a function object.
 *
 * Creates a scanner, compiler, and parser, and initiates the compilation
 * process. Returns the compiled function or NULL if errors occurred.
 *
 * @param source The source code to compile.
 * @return The compiled function object, or NULL on error.
 */
ObjFunction* compile(const char* source)
{
  auto scanner = Scanner::getScanner();
  scanner->initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
}

/**
 * @brief Marks compiler objects as roots for garbage collection.
 *
 * Iterates through the compiler hierarchy and marks function objects as roots.
 */
void markCompilerRoots()
{
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}