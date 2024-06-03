#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.hpp"
#include "value.hpp"

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
  OP_NOT,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
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
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
} OpCode;

class Chunk
{
public:
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;

  Chunk();

  void initChunk();
  void writeChunk(uint8_t byte, int line);
  void freeChunk();
  int addConstant(Value value);
};

#endif