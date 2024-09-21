#include <iostream>
#include <random>

#include "vm.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "dispatcher.hpp"
#include "memory.hpp"
#include "object.hpp"

static Value appendNative(int argCount, Value* args)
{
  // Append a value to the end of a list increasing the list's length by 1
  if (argCount != 2 || !IS_LIST(args[0])) {
    // Handle error
  }
  ObjList* list = AS_LIST(args[0]);
  Value item = args[1];

  appendToList(list, item);
  return NIL_VAL;
}

static Value deleteNative(int argCount, Value* args)
{
  // Delete an item from a list at the given index.
  if (argCount != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
    // Handle error
  }

  ObjList* list = AS_LIST(args[0]);
  int index = AS_NUMBER(args[1]);

  if (!isValidListIndex(list, index)) {
    // Handle error
  }

  deleteFromList(list, index);
  return NIL_VAL;
}

static Value strInput(int argCount, Value* args)
{
  // Append a value to the end of a list increasing the list's length by 1
  if (argCount > 1) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && !IS_STRING(args[0])) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && IS_STRING(args[0])) {
    printf("%s", AS_CSTRING(args[0]));
  }
  std::string s;
  std::cin >> s;
  return OBJ_VAL(copyString(s.c_str(), s.size()));
}

static Value charInput(int argCount, Value* args)
{
  // Append a value to the end of a list increasing the list's length by 1
  if (argCount > 1) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && !IS_STRING(args[0])) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && IS_STRING(args[0])) {
    printf("%s", AS_CSTRING(args[0]));
  }
  char s[1];
  scanf("%c", &s[0]);  // TODO: Handle Errors
  return OBJ_VAL(copyString(s, 1));
}

static Value intInput(int argCount, Value* args)
{
  if (argCount > 1) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && !IS_STRING(args[0])) {
    // Handle error
    exit(0);
  }
  if (argCount == 1 && IS_STRING(args[0])) {
    printf("%s", AS_CSTRING(args[0]));
  }

  double x;
  scanf("%lf", &x);  // TODO: Handle Errors
  return NUMBER_VAL(x);
}

static Value objLength(int argCount, Value* args)
{
  if (argCount != 1) {
    exit(0);
  }
  if (!IS_STRING(args[0]) && !IS_LIST(args[0])) {
    // Handle error
    exit(0);
  }
  if (IS_STRING(args[0])) {
    auto x = AS_STRING(args[0]);
    auto len = static_cast<double>(x->length);
    return NUMBER_VAL(len);
  }

  if (IS_LIST(args[0])) {
    auto x = AS_LIST(args[0]);
    auto len = static_cast<double>(x->count);
    return NUMBER_VAL(len);
  }

  return NUMBER_VAL(-1);
}

/**
 * @brief Native function to return the current system time as a number.
 *
 * This function implements the built-in `clock` function, which returns the
 * current system time as a number representing the elapsed CPU time since
 * program start. The returned value is divided by `CLOCKS_PER_SEC` to obtain a
 * floating-point representation of seconds.
 *
 * @param argCount The number of arguments passed to the function (ignored).
 * @param args The arguments passed to the function (ignored).
 * @return A number value representing the current system time in seconds.
 */
static Value clockNative(int argCount, Value* args)
{
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/**
 * @brief Native function to generate a random number.
 *
 * This function implements a built-in `rand` function, returning a random
 * number between 0 and RAND_MAX. The returned value is cast to a double for
 * compatibility with the Value type.
 *
 * @param argCount The number of arguments passed to the function (ignored).
 * @param args The arguments passed to the function (ignored).
 * @return A number value representing a random number.
 */
static Value randNative(int argCount, Value* args)
{
  auto rand_val = static_cast<double>(random());
  return NUMBER_VAL(rand_val);
}

/**
 * @brief Captures a local variable as an upvalue.
 *
 * Creates a new upvalue object for the given local variable if it doesn't
 * already exist. Updates the open upvalues list accordingly.
 *
 * @param local A pointer to the local variable.
 * @return The created or existing upvalue object.
 */
static ObjUpvalue* captureUpvalue(Value* local)
{
  // note source of failure
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  ObjUpvalue* prevUpvalue = NULL;
  auto upvalue = vm->openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  auto createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm->openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }
  return createdUpvalue;
}

/**
 * @brief Closes upvalues that are no longer in scope.
 *
 * Iterates over the open upvalues and closes any that are no longer accessible
 * by the current stack frame. The closed upvalue's value is captured and stored
 * in the upvalue itself.
 *
 * @param last A pointer to the last active local variable on the stack.
 */
static void closeUpvalues(Value* last)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  while (vm->openUpvalues != NULL && vm->openUpvalues->location >= last) {
    auto upvalue = vm->openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm->openUpvalues = upvalue->next;
  }
}

/**
 * @brief Constructs a new virtual machine instance.
 */
VM::VM() {}

/**
 * @brief Initializes the virtual machine.
 *
 * Resets the stack, initializes memory management, object lists, garbage
 * collection, string and global tables, and defines built-in native functions.
 */
void VM::initVM()
{
  this->resetStack();
  this->objects = NULL;
  this->bytesAllocated = 0;
  this->nextGC = 1024 * 1024;

  this->grayCount = 0;
  this->grayCapacity = 0;
  this->grayStack = NULL;

  this->strings.initTable();
  this->globals.initTable();

  this->finishStackCount = 0;

  this->initString = copyString("init", 4);

  defineNative("clock", clockNative);
  defineNative("rand", randNative);
  defineNative("append", appendNative);
  defineNative("delete", deleteNative);
  defineNative("int_input", intInput);
  defineNative("str_input", strInput);
  defineNative("char_input", charInput);
  defineNative("len", objLength);
}

/**
 * @brief Frees the resources allocated by the virtual machine.
 *
 * Deallocates memory used by global and string tables, and frees all allocated
 * objects.
 */
void VM::freeVM()
{
  this->globals.freeTable();
  this->strings.freeTable();
  this->initString = NULL;
  this->assigned = false;
  this->parent = NULL;
  freeObjects();
}

/**
 * @brief Interprets the given source code.
 *
 * Compiles the source code into a function, creates a closure for it, and
 * executes the function.
 *
 * @param source The source code to interpret.
 * @return The interpretation result, indicating success, compile error, or
 * runtime error.
 */
InterpretResult VM::interpret(const char* source)
{
  auto function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  auto closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);
  return run();
}

/**
 * @brief Defines a method for a class.
 *
 * Takes the current method (at the top of the stack) and the class object
 * (below it) and adds the method to the class's method table with the given
 * name.
 *
 * @param name The name of the method.
 */
void VM::defineMethod(ObjString* name)
{
  auto method = peek(0);
  auto klass = AS_CLASS(peek(1));
  klass->methods.tableSet(name, method);
  pop();
}

/**
 * @brief Concatenates two strings.
 *
 * Pops two string objects from the stack, concatenates them, and pushes the
 * resulting string onto the stack.
 *
 * This function assumes that the top two elements on the stack are string
 * objects.
 */
void VM::concatenate()
{
  auto b = AS_STRING(peek(0));
  auto a = AS_STRING(peek(1));

  auto length = a->length + b->length;
  auto chars = ALLOCATE<char>(length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  auto result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

/**
 * @brief Binds a method to an object instance.
 *
 * Creates a bound method object by taking the current object (receiver) and the
 * method from the class. If the method is not found in the class, a runtime
 * error is raised.
 *
 * @param klass The class of the object.
 * @param name The name of the method to bind.
 * @return `true` if the method was bound successfully, `false` otherwise.
 */
bool VM::bindMethod(ObjClass* klass, ObjString* name)
{
  Value method;
  if (!klass->methods.tableGet(name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  auto bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

/**
 * @brief Executes the bytecode in the current call frame.
 *
 * This is the main interpreter loop that fetches and executes instructions
 * until the end of the function. Handles various opcodes, stack manipulation,
 * function calls, returns, and error handling.
 *
 * @return The interpretation result, indicating success, compile error, or
 * runtime error.
 */
InterpretResult VM::run()
{
  auto frame = &this->frames[this->frameCount - 1];

  void* targets[] = {&&OP_CONSTANT_INSTRCTN,      &&OP_NIL_INSTRCTN,
                     &&OP_TRUE_INSTRCTN,          &&OP_FALSE_INSTRCTN,
                     &&OP_EQUAL_INSTRCTN,         &&OP_GREATER_INSTRCTN,
                     &&OP_LESS_INSTRCTN,          &&OP_RETURN_INSTRCTN,
                     &&OP_NEGATE_INSTRCTN,        &&OP_ADD_INSTRCTN,
                     &&OP_SUBTRACT_INSTRCTN,      &&OP_MULTIPLY_INSTRCTN,
                     &&OP_DIVIDE_INSTRCTN,        &&OP_MODULUS_INSTRCTN,
                     &&OP_NOT_INSTRCTN,           &&OP_PRINT_INSTRCTN,
                     &&OP_JUMP_INSTRCTN,          &&OP_JUMP_IF_FALSE_INSTRCTN,
                     &&OP_LOOP_INSTRCTN,          &&OP_CALL_INSTRCTN,
                     &&OP_INVOKE_INSTRCTN,        &&OP_SUPER_INVOKE_INSTRCTN,
                     &&OP_CLOSURE_INSTRCTN,       &&OP_GET_UPVALUE_INSTRCTN,
                     &&OP_SET_UPVALUE_INSTRCTN,   &&OP_GET_PROPERTY_INSTRCTN,
                     &&OP_SET_PROPERTY_INSTRCTN,  &&OP_POP_INSTRCTN,
                     &&OP_GET_LOCAL_INSTRCTN,     &&OP_SET_LOCAL_INSTRCTN,
                     &&OP_DEFINE_GLOBAL_INSTRCTN, &&OP_CLOSE_UPVALUE_INSTRCTN,
                     &&OP_CLASS_INSTRCTN,         &&OP_INHERIT_INSTRCTN,
                     &&OP_GET_SUPER_INSTRCTN,     &&OP_METHOD_INSTRCTN,
                     &&OP_GET_GLOBAL_INSTRCTN,    &&OP_SET_GLOBAL_INSTRCTN,
                     &&OP_BUILD_LIST_INSTRCTN,    &&OP_INDEX_GET_INSTRCTN,
                     &&OP_INDEX_SET_INSTRCTN,     &&OP_FINISH_BEGIN_INSTRCTN,
                     &&OP_FINISH_END_INSTRCTN,    &&OP_ASYNC_BEGIN_INSTRCTN,
                     &&OP_ASYNC_END_INSTRCTN};

  const auto READ_BYTE = [&frame]() { return *frame->ip++; };
#ifdef DEBUG_TRACE_EXECUTION
#  define NEXT_INSTRCTN() \
    do { \
      if (this->parent != NULL) { \
        auto thread_id = \
            std::hash<std::thread::id> {}(std::this_thread::get_id()); \
        std::cout << thread_id << std::endl; \
        printf("          "); \
        for (Value* slot = this->stack; slot < this->stackTop; slot++) { \
          printf("[ "); \
          printValue(*slot); \
          printf(" ]"); \
        } \
        printf("\n"); \
        disassembleInstruction( \
            &frame->closure->function->chunk, \
            (int)(frame->ip - frame->closure->function->chunk.code)); \
      } \
      goto* targets[READ_BYTE()]; \
    } while (0)

#else
#  define NEXT_INSTRCTN() goto* targets[READ_BYTE()];
#endif

  const auto READ_SHORT = [&frame]()
  {
    frame->ip += 2;
    return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
  };

  const auto READ_CONSTANT = [&frame, READ_BYTE]
  { return frame->closure->function->chunk.constants.values[READ_BYTE()]; };

  const auto READ_STRING = [READ_CONSTANT]
  { return AS_STRING(READ_CONSTANT()); };

  const auto BINARY_OP = [this](char op)
  {
    if (op == '+') {
      if (IS_STRING(this->peek(0)) && IS_STRING(this->peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        auto b = AS_NUMBER(pop());
        auto a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
    } else if (op == '-') {
      if (IS_STRING(this->peek(0)) && IS_STRING(this->peek(1))) {
        auto a = AS_STRING(this->peek(0));
        auto b = AS_STRING(this->peek(1));

        if (b->length != 1 || a->length != 1) {
          runtimeError("Operands must be two characters");
          return INTERPRET_RUNTIME_ERROR;
        }
        auto diff = b->chars[0] - a->chars[0];
        auto double_diff = static_cast<double>(diff);
        pop();
        pop();
        push(NUMBER_VAL(double_diff));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        auto b = AS_NUMBER(pop());
        auto a = AS_NUMBER(pop());
        push(NUMBER_VAL(a - b));
      } else {
        runtimeError("Operands must be two numbers or two chars");
        return INTERPRET_RUNTIME_ERROR;
      }
    } else {
      if (!IS_NUMBER(this->peek(0)) || !IS_NUMBER(this->peek(1))) {
        runtimeError("Operands must be numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      auto b = AS_NUMBER(pop());
      auto a = AS_NUMBER(pop());
      switch (op) {
        case '+':
          push(NUMBER_VAL(a + b));
          break;
        case '-':
          push(NUMBER_VAL(a - b));
          break;
        case '*':
          push(NUMBER_VAL(a * b));
          break;
        case '/': {
          push(NUMBER_VAL(a / b));
          break;
        }
        case '%': {
          int i_a = static_cast<int>(a);
          int i_b = static_cast<int>(b);
          double response = static_cast<double>(i_a % i_b);
          push(NUMBER_VAL(response));
          break;
        }
        case '>':
          push(BOOL_VAL(a > b));
          break;
        case '<':
          push(BOOL_VAL(a < b));
          break;
        default:
          runtimeError("Invalid Binary Operator.");
          return INTERPRET_RUNTIME_ERROR;
          break;
      }
    }
    return INTERPRET_CONTINUE;
  };

  NEXT_INSTRCTN();

OP_CONSTANT_INSTRCTN : {
  auto constant = READ_CONSTANT();
  this->push(constant);
  NEXT_INSTRCTN();
}

OP_RETURN_INSTRCTN : {
  auto result = pop();
  closeUpvalues(frame->slots);
  this->frameCount--;
  if (this->frameCount == 0) {
    pop();
    return INTERPRET_OK;
  }

  this->stackTop = frame->slots;
  push(result);
  frame = &this->frames[this->frameCount - 1];
  NEXT_INSTRCTN();
}

OP_NEGATE_INSTRCTN : {
  if (!IS_NUMBER(this->peek(0))) {
    runtimeError("Operand must be a number.");
    return INTERPRET_RUNTIME_ERROR;
  }
  push(NUMBER_VAL(-AS_NUMBER(pop())));
  NEXT_INSTRCTN();
}

OP_ADD_INSTRCTN : {
  if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
    concatenate();
  } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
    auto b = AS_NUMBER(pop());
    auto a = AS_NUMBER(pop());
    push(NUMBER_VAL(a + b));
  } else {
    runtimeError("Operands must be two numbers or two strings.");
    return INTERPRET_RUNTIME_ERROR;
  }
  NEXT_INSTRCTN();
}

OP_SUBTRACT_INSTRCTN : {
  auto res = BINARY_OP('-');
  if (res != INTERPRET_CONTINUE) {
    return res;
  }
  NEXT_INSTRCTN();
}

OP_MULTIPLY_INSTRCTN : {
  auto res = BINARY_OP('*');
  if (res != INTERPRET_CONTINUE) {
    return res;
  }
  NEXT_INSTRCTN();
}

OP_DIVIDE_INSTRCTN : {
  auto res = BINARY_OP('/');
  if (res != INTERPRET_CONTINUE) {
    return res;
  }
  NEXT_INSTRCTN();
}

OP_MODULUS_INSTRCTN : {
  auto res = BINARY_OP('%');
  if (res != INTERPRET_CONTINUE) {
    return res;
  }
  NEXT_INSTRCTN();
}

OP_NOT_INSTRCTN : {
  push(BOOL_VAL(isFalsey(pop())));
  NEXT_INSTRCTN();
}

OP_NIL_INSTRCTN : {
  push(NIL_VAL);
  NEXT_INSTRCTN();
}

OP_TRUE_INSTRCTN : {
  push(BOOL_VAL(true));
  NEXT_INSTRCTN();
}

OP_FALSE_INSTRCTN : {
  push(BOOL_VAL(false));
  NEXT_INSTRCTN();
}

OP_GREATER_INSTRCTN : {
  auto res = BINARY_OP('>');
  if (res != INTERPRET_CONTINUE) {
    return res;
    NEXT_INSTRCTN();
  }
  NEXT_INSTRCTN();
}

OP_LESS_INSTRCTN : {
  auto res = BINARY_OP('<');
  if (res != INTERPRET_CONTINUE) {
    return res;
  }
  NEXT_INSTRCTN();
}

OP_EQUAL_INSTRCTN : {
  auto b = pop();
  auto a = pop();
  push(BOOL_VAL(valuesEqual(a, b)));
  NEXT_INSTRCTN();
}

OP_METHOD_INSTRCTN : {
  defineMethod(READ_STRING());
  NEXT_INSTRCTN();
}

OP_CLASS_INSTRCTN : {
  push(OBJ_VAL(newClass(READ_STRING())));
  NEXT_INSTRCTN();
}

OP_CLOSURE_INSTRCTN : {
  auto function = AS_FUNCTION(READ_CONSTANT());
  auto closure = newClosure(function);
  push(OBJ_VAL(closure));
  for (int i = 0; i < closure->upvalueCount; i++) {
    auto isLocal = READ_BYTE();
    auto index = READ_BYTE();
    if (isLocal) {
      closure->upvalues[i] = captureUpvalue(frame->slots + index);
    } else {
      closure->upvalues[i] = frame->closure->upvalues[index];
    }
  }
  NEXT_INSTRCTN();
}

OP_PRINT_INSTRCTN : {
  printValue(pop());
  printf("\n");
  NEXT_INSTRCTN();
}

OP_CALL_INSTRCTN : {
  auto argCount = READ_BYTE();
  if (!callValue(peek(argCount), argCount)) {
    return INTERPRET_RUNTIME_ERROR;
  }
  frame = &this->frames[this->frameCount - 1];
  NEXT_INSTRCTN();
}

OP_POP_INSTRCTN : {
  pop();
  NEXT_INSTRCTN();
}
OP_DEFINE_GLOBAL_INSTRCTN : {
  auto name = READ_STRING();
  this->globals.tableSet(name, peek(0));
  pop();
  NEXT_INSTRCTN();
}

OP_GET_GLOBAL_INSTRCTN : {
  auto name = READ_STRING();
  Value value;
  if (!this->globals.tableGet(name, &value)) {
    runtimeError("Undefined variable '%s'.", name->chars);
    return INTERPRET_RUNTIME_ERROR;
  }
  push(value);
  NEXT_INSTRCTN();
}

OP_GET_UPVALUE_INSTRCTN : {
  auto slot = READ_BYTE();
  push(*frame->closure->upvalues[slot]->location);
  NEXT_INSTRCTN();
}

OP_SET_UPVALUE_INSTRCTN : {
  auto slot = READ_BYTE();
  *frame->closure->upvalues[slot]->location = peek(0);
  NEXT_INSTRCTN();
}

OP_GET_PROPERTY_INSTRCTN : {
  if (!IS_INSTANCE(peek(0))) {
    runtimeError("Only instances have properties.");
    return INTERPRET_RUNTIME_ERROR;
  }

  auto instance = AS_INSTANCE(peek(0));
  auto name = READ_STRING();

  Value value;
  if (instance->fields.tableGet(name, &value)) {
    pop();  // Instance.
    push(value);
    NEXT_INSTRCTN();
  }
  if (!bindMethod(instance->klass, name)) {
    return INTERPRET_RUNTIME_ERROR;
  }
  NEXT_INSTRCTN();
}

OP_SET_PROPERTY_INSTRCTN : {
  if (!IS_INSTANCE(peek(1))) {
    runtimeError("Only instances have fields.");
    return INTERPRET_RUNTIME_ERROR;
  }
  auto instance = AS_INSTANCE(peek(1));
  instance->fields.tableSet(READ_STRING(), peek(0));
  auto value = pop();
  pop();
  push(value);
  NEXT_INSTRCTN();
}

OP_CLOSE_UPVALUE_INSTRCTN : {
  closeUpvalues(this->stackTop - 1);
  pop();
  NEXT_INSTRCTN();
}
OP_GET_LOCAL_INSTRCTN : {
  auto slot = READ_BYTE();
  push(frame->slots[slot]);
  NEXT_INSTRCTN();
}

OP_SET_LOCAL_INSTRCTN : {
  auto slot = READ_BYTE();
  frame->slots[slot] = peek(0);
  NEXT_INSTRCTN();
}

OP_JUMP_IF_FALSE_INSTRCTN : {
  auto offset = READ_SHORT();
  if (isFalsey(peek(0)))
    frame->ip += offset;
  NEXT_INSTRCTN();
}

OP_SET_GLOBAL_INSTRCTN : {
  auto name = READ_STRING();
  if (this->globals.tableSet(name, peek(0))) {
    this->globals.tableDelete(name);
    runtimeError("Undefined variable '%s'.", name->chars);
    return INTERPRET_RUNTIME_ERROR;
  }
  NEXT_INSTRCTN();
}

OP_LOOP_INSTRCTN : {
  auto offset = READ_SHORT();
  frame->ip -= offset;
  NEXT_INSTRCTN();
}

OP_JUMP_INSTRCTN : {
  auto offset = READ_SHORT();
  frame->ip += offset;
  NEXT_INSTRCTN();
}

OP_INHERIT_INSTRCTN : {
  auto superclass = peek(1);
  if (!IS_CLASS(superclass)) {
    runtimeError("Superclass must be a class.");
    return INTERPRET_RUNTIME_ERROR;
  }
  auto subclass = AS_CLASS(peek(0));
  tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
  pop();  // Subclass.
  NEXT_INSTRCTN();
}

OP_INVOKE_INSTRCTN : {
  auto method = READ_STRING();
  auto argCount = READ_BYTE();
  if (!invoke(method, argCount)) {
    return INTERPRET_RUNTIME_ERROR;
  }
  frame = &this->frames[this->frameCount - 1];
  NEXT_INSTRCTN();
}

OP_GET_SUPER_INSTRCTN : {
  auto name = READ_STRING();
  auto superclass = AS_CLASS(pop());
  if (!bindMethod(superclass, name)) {
    return INTERPRET_RUNTIME_ERROR;
  }
  NEXT_INSTRCTN();
}

OP_SUPER_INVOKE_INSTRCTN : {
  auto method = READ_STRING();
  auto argCount = READ_BYTE();
  auto superclass = AS_CLASS(pop());
  if (!invokeFromClass(superclass, method, argCount)) {
    return INTERPRET_RUNTIME_ERROR;
  }
  frame = &this->frames[this->frameCount - 1];
  NEXT_INSTRCTN();
}

OP_BUILD_LIST_INSTRCTN : {
  ObjList* list = newList();
  uint8_t itemCount = READ_BYTE();
  push(OBJ_VAL(list));  // So list isn't sweeped by GC in appendToList
  for (int i = itemCount; i > 0; i--) {
    appendToList(list, peek(i));
  }
  pop();
  while (itemCount-- > 0) {
    pop();
  }
  push(OBJ_VAL(list));
  NEXT_INSTRCTN();
}

OP_INDEX_GET_INSTRCTN : {
  Value st_index = pop();
  Value st_obj = pop();
  Value result;

  if (!IS_LIST(st_obj) && !IS_STRING(st_obj)) {
    runtimeError("Invalid type to index into.");
    return INTERPRET_RUNTIME_ERROR;
  }
  if (!IS_NUMBER(st_index)) {
    runtimeError("List index is not a number.");
    return INTERPRET_RUNTIME_ERROR;
  }
  if (IS_LIST(st_obj)) {
    ObjList* list = AS_LIST(st_obj);
    int index = AS_NUMBER(st_index);
    if (!isValidListIndex(list, index)) {
      runtimeError("List index out of range.");
      return INTERPRET_RUNTIME_ERROR;
    }

    result = indexFromList(list, index);
    push(result);
  } else if (IS_STRING(st_obj)) {
    ObjString* string = AS_STRING(st_obj);
    int index = AS_NUMBER(st_index);
    if (!isValidStringIndex(string, index)) {
      runtimeError("String index out of range");
      return INTERPRET_RUNTIME_ERROR;
    }
    result = indexFromString(string, index);
    push(result);
  }

  NEXT_INSTRCTN();
}

OP_INDEX_SET_INSTRCTN : {
  // Stack before: [list, index, item] and after: [item]
  Value st_item = pop();
  Value st_index = pop();
  Value st_obj = pop();

  if (!IS_LIST(st_obj) && !IS_STRING(st_obj)) {
    runtimeError("Cannot store value in a non-list.");
    return INTERPRET_RUNTIME_ERROR;
  }

  if (!IS_NUMBER(st_index)) {
    runtimeError("List index is not a number.");
    return INTERPRET_RUNTIME_ERROR;
  }

  if (IS_LIST(st_obj)) {
    ObjList* list = AS_LIST(st_obj);
    int index = AS_NUMBER(st_index);

    if (!isValidListIndex(list, index)) {
      runtimeError("Invalid list index.");
      return INTERPRET_RUNTIME_ERROR;
    }

    storeToList(list, index, st_item);
    push(st_item);
  } else if (IS_STRING(st_obj)) {
    ObjString* string = AS_STRING(st_obj);
    int index = AS_NUMBER(st_index);

    if (!isValidStringIndex(string, index)) {
      runtimeError("Invalid list index.");
      return INTERPRET_RUNTIME_ERROR;
    }

    ObjString* item = AS_STRING(st_item);

    if (item->length > 1) {
      runtimeError("Invalid assignment value");
      return INTERPRET_RUNTIME_ERROR;
    }

    storeToString(string, index, item);
    push(st_item);
  }

  NEXT_INSTRCTN();
}
OP_FINISH_BEGIN_INSTRCTN : {
  this->finishStackCount++;
  NEXT_INSTRCTN();
}
OP_FINISH_END_INSTRCTN : {
  for (auto& thread : this->finishStack[this->finishStackCount]) {
    if (thread->joinable()) {
      thread->join();
    }
  }
  this->finishStack[this->finishStackCount].clear();
  this->finishStackCount--;
  NEXT_INSTRCTN();
}
OP_ASYNC_BEGIN_INSTRCTN : {
  // Prep Thread VM to execute
  // Note: Skip jump in bytecode
  auto dispatcher = Dispatcher::getDispatcher();
  auto new_thread = dispatcher->asyncBegin();
  // new_thread.join();

  this->finishStack[this->finishStackCount].push_back(&new_thread);
  // Start Next line of execution
  auto offset = READ_SHORT();
  frame->ip += offset;
  NEXT_INSTRCTN();
}
OP_ASYNC_END_INSTRCTN : {
  auto dispatcher = Dispatcher::getDispatcher();
  dispatcher->freeVM();
  pop();
  return INTERPRET_OK;
}
#undef NEXT_INSTRCTN
}

/**
 * @brief Gets the singleton virtual machine instance.
 *
 * Returns a pointer to the globally accessible virtual machine object.
 *
 * @return A pointer to the virtual machine instance.
 */
// VM* VM::getVM()
// {
//   return vm;
// }

/**
 * @brief Resets the virtual machine's stack.
 *
 * Clears the stack, frame count, and open upvalues, preparing the VM for a new
 * execution.
 */
void VM::resetStack()
{
  this->stackTop = this->stack;
  this->frameCount = 0;
  this->openUpvalues = NULL;
}

/**
 * @brief Pushes a value onto the top of the stack.
 *
 * Increments the stack pointer and stores the given value at the new top of the
 * stack.
 *
 * @param value The value to be pushed onto the stack.
 */
void VM::push(Value value)
{
  *this->stackTop = value;
  this->stackTop++;
}

/**
 * @brief Pops a value from the top of the stack.
 *
 * Decrements the stack pointer and returns the value at the previous top of the
 * stack.
 *
 * @return The popped value.
 */
Value VM::pop()
{
  this->stackTop--;
  return *this->stackTop;
}

/**
 * @brief Peeks at a value on the stack without removing it.
 *
 * Returns the value at a specified offset from the top of the stack.
 *
 * @param distance The offset from the top of the stack.
 * @return The value at the specified offset.
 */
Value VM::peek(int distance)
{
  return this->stackTop[-1 - distance];
}

/**
 * @brief Calls a closure.
 *
 * Creates a new call frame for the closure, checks the argument count, and
 * pushes the return address onto the stack.
 *
 * @param closure The closure to call.
 * @param argCount The number of arguments passed to the function.
 * @return `true` if the call was successful, `false` if an error occurred.
 */
bool VM::call(ObjClosure* closure, int argCount)
{
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
                 closure->function->arity,
                 argCount);
    return false;
  }

  if (this->frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &this->frames[this->frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = this->stackTop - argCount - 1;
  return true;
}

/**
 * @brief Calls a callable value.
 *
 * Determines the type of the callable value and dispatches the call
 * accordingly. Handles bound methods, class constructors, closures, and native
 * functions.
 *
 * @param callee The callable value to be invoked.
 * @param argCount The number of arguments passed to the function.
 * @return `true` if the call was successful, `false` if an error occurred.
 */
bool VM::callValue(Value callee, int argCount)
{
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        this->stackTop[-argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
      }
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        this->stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
        Value initializer;
        if (klass->methods.tableGet(this->initString, &initializer)) {
          return call(AS_CLOSURE(initializer), argCount);
        } else if (argCount != 0) {
          runtimeError("Expected 0 arguments but got %d.", argCount);
          return false;
        }

        return true;
      }
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argCount);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, this->stackTop - argCount);
        this->stackTop -= argCount + 1;
        push(result);
        return true;
      }
      default:
        break;  // Non-callable object type.
    }
  }
  this->runtimeError("Can only call functions and classes.");
  return false;
}

/**
 * @brief Invokes a method on a class instance.
 *
 * Looks up the specified method in the class's method table and calls it with
 * the given argument count.
 *
 * @param klass The class instance.
 * @param name The name of the method to invoke.
 * @param argCount The number of arguments passed to the method.
 * @return `true` if the method was invoked successfully, `false` if an error
 * occurred.
 */
bool VM::invokeFromClass(ObjClass* klass, ObjString* name, int argCount)
{
  Value method;
  if (!klass->methods.tableGet(name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), argCount);
}

/**
 * @brief Invokes a method on an object.
 *
 * Looks up the method by name in the object's instance variables or its class's
 * methods. Calls the method with the given arguments.
 *
 * @param name The name of the method to invoke.
 * @param argCount The number of arguments passed to the method.
 * @return `true` if the method was invoked successfully, `false` if an error
 * occurred.
 */
bool VM::invoke(ObjString* name, int argCount)
{
  Value receiver = peek(argCount);
  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }
  ObjInstance* instance = AS_INSTANCE(receiver);

  Value value;
  if (instance->fields.tableGet(name, &value)) {
    this->stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }
  return invokeFromClass(instance->klass, name, argCount);
}

/**
 * @brief Checks if a value is considered falsey.
 *
 * Determines whether a given value is considered false in the language's
 * context. Currently, `nil` and `false` are considered falsey.
 *LFwise.
 */
bool VM::isFalsey(Value value)
{
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/**
 * @brief Reports a runtime error and terminates execution.
 *
 * Prints an error message to stderr, including the error message, the current
 * line number, and the call stack. Resets the virtual machine's state after
 * printing the error.
 *
 * @param format The format string for the error message.
 * @param ... Variable arguments for the format string.
 */
void VM::runtimeError(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = this->frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &this->frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

/**
 * @brief Defines a native function in the global environment.
 *
 * Creates a string for the function name, creates a native function object, and
 * adds it to the global table.
 *
 * @param name The name of the native function.
 * @param function The native function implementation.
 */
void VM::defineNative(const char* name, NativeFn function)
{
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  this->globals.tableSet(AS_STRING(this->stack[0]), this->stack[1]);
  pop();
  pop();
}

void VM::copyParent(VM* parent)
{
  if (parent != NULL) {
    std::copy(parent->frames, parent->frames + 2048, this->frames);
    std::copy(parent->stack, parent->stack + STACK_MAX, this->stack);
    auto diff = parent->stackTop - stack;
    this->stackTop = this->stack + diff;
    this->frameCount = parent->frameCount;
    this->parent = parent;
    // TODO: check if this causes BT in enclosing variables
    this->strings.initTable();
    this->globals.initTable();

    this->openUpvalues = parent->openUpvalues;

    // Do not mess with GC in child variables
    this->bytesAllocated = 0;
    this->nextGC = 1024 * 1024;

    this->grayCount = 0;
    this->grayCapacity = 0;
    this->grayStack = NULL;

    this->finishStackCount = 0;
    this->initString = copyString("init", 4);

    defineNative("clock", clockNative);
    defineNative("rand", randNative);
    defineNative("append", appendNative);
    defineNative("delete", deleteNative);
    defineNative("int_input", intInput);
    defineNative("str_input", strInput);
    defineNative("char_input", charInput);
    defineNative("len", objLength);

  } else {
    this->initVM();
  }
  this->assigned = true;
}

VM* VM::vm = new VM;
