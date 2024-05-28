#include "vm.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "object.hpp"

VM::VM()
{
  this->initVM();
}

void VM::initVM()
{
  this->resetStack();
  this->objects = NULL;
  this->strings.initTable();
  this->globals.initTable();
}
void VM::freeVM()
{
  freeObjects();
  this->globals.freeTable();
  this->strings.freeTable();
}

InterpretResult VM::interpret(const char* source)
{
  auto vm = VM::getVM();
  Chunk chunk;
  chunk.initChunk();

  if (!compile(source, &chunk)) {
    chunk.freeChunk();
    return INTERPRET_COMPILE_ERROR;
  }

  vm->chunk = &chunk;
  vm->ip = vm->chunk->code;

  InterpretResult result = run();

  chunk.freeChunk();
  return result;
}

void VM::concatenate()
{
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}

InterpretResult VM::run()
{
#define READ_BYTE() (*(this->ip++))
#define READ_CONSTANT() (this->chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() \
  (this->vm->ip += 2, (uint16_t)((this->vm->ip[-2] << 8) | this->vm->ip[-1]))
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
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
        return INTERPRET_OK;
      }
      case OP_NEGATE: {
        if (!IS_NUMBER(this->peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
        break;
      }
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(NUMBER_VAL, -);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(NUMBER_VAL, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        break;
      }
      case OP_NOT: {
        push(BOOL_VAL(isFalsey(pop())));
        break;
      }
      case OP_NIL: {
        push(NIL_VAL);
        break;
      }
      case OP_TRUE: {
        push(BOOL_VAL(true));
        break;
      }
      case OP_FALSE: {
        push(BOOL_VAL(false));
        break;
      }
      case OP_GREATER:
        BINARY_OP(BOOL_VAL, >);
        break;
      case OP_LESS:
        BINARY_OP(BOOL_VAL, <);
        break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_POP:
        pop();
        break;
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        this->globals.tableSet(name, peek(0));
        pop();
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!this->globals.tableGet(name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(this->vm->stack[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        this->vm->stack[slot] = peek(0);
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0)))
          this->vm->ip += offset;
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (this->globals.tableSet(name, peek(0))) {
          this->globals.tableDelete(name);
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        this->vm->ip -= offset;
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        this->vm->ip += offset;
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
#undef READ_SHORT
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

Value VM::peek(int distance)
{
  return this->stackTop[-1 - distance];
}

bool VM::isFalsey(Value value)
{
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void VM::runtimeError(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = this->ip - this->chunk->code - 1;
  int line = this->chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

VM* VM::vm = new VM;