#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.hpp"
#include "memory.hpp"
#include "value.hpp"

typedef enum
{
  OP_CONSTANT,
  OP_RETURN,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
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