#ifndef clox_vm_h
#define clox_vm_h

#include "object.hpp"
#include "table.hpp"

constexpr int FRAMES_MAX = 64;
constexpr int STACK_MAX = (FRAMES_MAX * UINT8_COUNT);

/**
 * @brief Represents a call frame on the virtual machine's stack.
 *
 * A call frame stores information about an active function call, including the
 * closure, instruction pointer, and local variables.
 */
class CallFrame
{
public:
  /**
   * @brief The closure associated with the call frame.
   */
  ObjClosure* closure;

  /**
   * @brief The current instruction pointer within the closure's chunk.
   */
  uint8_t* ip;

  /**
   * @brief A pointer to the base of the local variables for this frame.
   */
  Value* slots;
};

/**
 * @brief Enumeration representing the result of interpreting code.
 *
 * Defines possible return values from the interpreter.
 *
 * @enum InterpretResult
 * @value INTERPRET_OK Interpretation was successful.
 * @value INTERPRET_COMPILE_ERROR A compilation error occurred.
 * @value INTERPRET_RUNTIME_ERROR A runtime error occurred.
 * @value INTERPRET_CONTINUE Interpretation is continuing (for internal use).
 */
typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
  INTERPRET_CONTINUE
} InterpretResult;

class VM
{
private:
  /**
   * @brief Constructs a new virtual machine instance.
   */
  VM();

  /**
   * @brief Resets the virtual machine's stack.
   *
   * Clears the stack, frame count, and open upvalues, preparing the VM for a
   * new execution.
   */
  void resetStack();

  /**
   * @brief Concatenates two strings.
   *
   * Pops two string objects from the stack, concatenates them, and pushes the
   * resulting string onto the stack.
   *
   * This function assumes that the top two elements on the stack are string
   * objects.
   */
  void concatenate();

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
  bool call(ObjClosure* closure, int argCount);

  /**
   * @brief Calls a callable value.
   *
   * Determines the type of the callable value and dispatches the call
   * accordingly. Handles bound methods, class constructors, closures, and
   * native functions.
   *
   * @param callee The callable value to be invoked.
   * @param argCount The number of arguments passed to the function.
   * @return `true` if the call was successful, `false` if an error occurred.
   */
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

  /**
   * @brief Initializes the virtual machine.
   *
   * Resets the stack, initializes memory management, object lists, garbage
   * collection, string and global tables, and defines built-in native
   * functions.
   */
  void initVM();

  /**
   * @brief Frees the resources allocated by the virtual machine.
   *
   * Deallocates memory used by global and string tables, and frees all
   * allocated objects.
   */
  void freeVM();

  /**
   * @brief Gets the singleton virtual machine instance.
   *
   * Returns a pointer to the globally accessible virtual machine object.
   *
   * @return A pointer to the virtual machine instance.
   */
  static VM* getVM();

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
  InterpretResult interpret(const char* source);

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
  InterpretResult run();

  /**
   * @brief Pushes a value onto the top of the stack.
   *
   * Increments the stack pointer and stores the given value at the new top of
   * the stack.
   *
   * @param value The value to be pushed onto the stack.
   */
  void push(Value value);

  /**
   * @brief Pops a value from the top of the stack.
   *
   * Decrements the stack pointer and returns the value at the previous top of
   * the stack.
   *
   * @return The popped value.
   */
  Value pop();

  /**
   * @brief Peeks at a value on the stack without removing it.
   *
   * Returns the value at a specified offset from the top of the stack.
   *
   * @param distance The offset from the top of the stack.
   * @return The value at the specified offset.
   */
  Value peek(int distance);

  /**
   * @brief Checks if a value is considered falsey.
   *
   * Determines whether a given value is considered false in the language's
   * context. Currently, `nil` and `false` are considered falsey.
   *
   * @param value The value to check.
   * @return `true` if the value is falsey, `false` otherwise.
   */
  bool isFalsey(Value value);

  /**
   * @brief Defines a native function in the global environment.
   *
   * Creates a string for the function name, creates a native function object,
   * and adds it to the global table.
   *
   * @param name The name of the native function.
   * @param function The native function implementation.
   */
  void defineNative(const char* name, NativeFn function);

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
  void runtimeError(const char* format, ...);

  /**
   * @brief Defines a method for a class.
   *
   * Takes the current method (at the top of the stack) and the class object
   * (below it) and adds the method to the class's method table with the given
   * name.
   *
   * @param name The name of the method.
   */
  void defineMethod(ObjString* name);

  /**
   * @brief Binds a method to an object instance.
   *
   * Creates a bound method object by taking the current object (receiver) and
   * the method from the class. If the method is not found in the class, a
   * runtime error is raised.
   *
   * @param klass The class of the object.
   * @param name The name of the method to bind.
   * @return `true` if the method was bound successfully, `false` otherwise.
   */
  bool bindMethod(ObjClass* klass, ObjString* name);

  /**
   * @brief Invokes a method on an object.
   *
   * Looks up the method by name in the object's instance variables or its
   * class's methods. Calls the method with the given arguments.
   *
   * @param name The name of the method to invoke.
   * @param argCount The number of arguments passed to the method.
   * @return `true` if the method was invoked successfully, `false` if an error
   * occurred.
   */
  bool invoke(ObjString* name, int argCount);

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
  bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);
};

#endif