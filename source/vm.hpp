#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.hpp"

#define STACK_MAX 256

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

class VM
{
private:
  VM();

  void resetStack();

public:
  static VM* vm;

  Chunk* chunk;
  uint8_t* ip;

  Value stack[STACK_MAX];
  Value* stackTop;

  void initVM();
  void freeVM();

  static VM* getVM();

  InterpretResult interpret(Chunk& chunk);
  InterpretResult run();

  void push(Value value);
  Value pop();
};

#endif