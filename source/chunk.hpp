#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.hpp"
#include "value.hpp"

/**
 * @brief List of OpCode supported by the Lox Virtual Machine
 */
typedef enum
{
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_RETURN,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULUS,
  OP_NOT,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_DEFINE_GLOBAL,
  OP_CLOSE_UPVALUE,
  OP_CLASS,
  OP_INHERIT,
  OP_GET_SUPER,
  OP_METHOD,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_BUILD_LIST,
  OP_INDEX_GET,
  OP_INDEX_SET
} OpCode;

/**
 * @brief Represents a chunk of compiled bytecode.
 *
 * A Chunk stores the bytecode instructions, line numbers, and constant values
 * for a function. It manages the bytecode as a dynamic array to accommodate
 * varying code sizes.
 */
class Chunk
{
public:
  /**
   * @brief The number of instructions in the chunk.
   */
  int count;

  /**
   * @brief The maximum capacity of the chunk.
   */
  int capacity;

  /**
   * @brief The array of bytecode instructions.
   */
  uint8_t* code;

  /**
   * @brief An array mapping instruction offsets to line numbers.
   */
  int* lines;

  /**
   * @brief An array of constant values used in the chunk.
   */
  ValueArray constants;

  /**
   * @brief Constructs a new, empty chunk.
   */
  Chunk();

  /**
   * @brief Initialises the parameters in the chunk
   */
  void initChunk();

  /**
   * @brief Writes a byte to the chunk
   *
   * @param byte Bytecode written to chunk
   * @param line Line number getting written to chunk
   */
  void writeChunk(uint8_t byte, int line);

  /**
   * @brief Free all resources associated with a chunk
   */
  void freeChunk();

  /**
   * @brief Add a value to the constants array
   *
   * @param value Value being appended
   * @return int The index of the appended element
   */
  int addConstant(Value value);
};

#endif