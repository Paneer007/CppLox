#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.hpp"
#include "table.hpp"

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
  void concatenate();

public:
  static VM* vm;

  Chunk* chunk;
  uint8_t* ip;

  Value stack[STACK_MAX];
  Value* stackTop;
  Table strings;
  Table globals;
  Obj* objects;

  void initVM();
  void freeVM();

  static VM* getVM();

  InterpretResult interpret(const char* source);
  InterpretResult run();

  void push(Value value);
  Value pop();
  Value peek(int distance);

  bool isFalsey(Value value);

  void runtimeError(const char* format, ...);
};

#endif