#include "vm.hpp"

#include <stdio.h>

#include "common.hpp"
#include "debug.hpp"

VM::VM()
{
  this->initVM();
}

void VM::initVM()
{
  this->resetStack();
}
void VM::freeVM() {}

InterpretResult VM::interpret(Chunk& chunk)
{
  this->chunk = &chunk;
  this->ip = this->chunk->code;
  return this->run();
}

InterpretResult VM::run()
{
#define READ_BYTE() (*(this->ip++))
#define READ_CONSTANT() (this->chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
  do { \
    double b = pop(); \
    double a = pop(); \
    push(a op b); \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = this->stack; slot < this->stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(this->chunk, (int)(this->ip - this->chunk->code));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        this->push(constant);
        break;
      }
      case OP_RETURN: {
        printValue(pop());
        printf("\n");
        return INTERPRET_OK;
      }
      case OP_NEGATE: {
        this->push(-pop());
        break;
      }
      case OP_ADD: {
        BINARY_OP(+);
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(-);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(*);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(/);
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

VM* VM::getVM()
{
  return vm;
}

void VM::resetStack()
{
  this->stackTop = this->stack;
}

void VM::push(Value value)
{
  *this->stackTop = value;
  this->stackTop++;
}

Value VM::pop()
{
  this->stackTop--;
  return *this->stackTop;
}

VM* VM::vm = new VM;