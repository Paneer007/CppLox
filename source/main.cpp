#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"

int main(int argc, const char* argv[])
{
  Chunk chunk;

  int constant = chunk.addConstant(1.2);
  chunk.writeChunk(OP_CONSTANT, 123);
  chunk.writeChunk(constant, 123);
  chunk.writeChunk(OP_RETURN, 123);
  disassembleChunk(&chunk, "test chunk");
  chunk.freeChunk();
  return 0;
}