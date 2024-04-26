#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "vm.hpp"

int main(int argc, const char* argv[])
{
  Chunk chunk;
  auto vm = VM::getVM();
  vm->initVM();
  int constant = chunk.addConstant(1.2);
  chunk.writeChunk(OP_CONSTANT, 123);
  chunk.writeChunk(constant, 123);

  constant = chunk.addConstant(3.4);
  chunk.writeChunk(OP_CONSTANT, 123);
  chunk.writeChunk(constant, 123);

  chunk.writeChunk(OP_ADD, 123);

  constant = chunk.addConstant(5.6);
  chunk.writeChunk(OP_CONSTANT, 123);
  chunk.writeChunk(constant, 123);

  chunk.writeChunk(OP_DIVIDE, 123);

  chunk.writeChunk(OP_NEGATE, 123);
  chunk.writeChunk(OP_RETURN, 123);
  vm->interpret(chunk);
  vm->freeVM();
  chunk.freeChunk();
  return 0;
}