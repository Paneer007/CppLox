#ifndef clox_vm_h
#define clox_vm_h

#include "object.hpp"
#include "table.hpp"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

class CallFrame
{
public:
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
};

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

  bool call(ObjClosure* closure, int argCount);
  bool callValue(Value callee, int argCount);

public:
  static VM* vm;

  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  Table strings;
  Table globals;
  ObjUpvalue* openUpvalues;
  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
  ObjString* initString;

  void initVM();
  void freeVM();

  static VM* getVM();

  InterpretResult interpret(const char* source);
  InterpretResult run();

  void push(Value value);
  Value pop();
  Value peek(int distance);

  bool isFalsey(Value value);
  void defineNative(const char* name, NativeFn function);
  void runtimeError(const char* format, ...);
  void defineMethod(ObjString* name);
  bool bindMethod(ObjClass* klass, ObjString* name);
  bool invoke(ObjString* name, int argCount);
  bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);
};

#endif