#ifndef clox_object_h
#define clox_object_h

#include "chunk.hpp"
#include "common.hpp"
#include "table.hpp"
#include "value.hpp"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_LIST(value) isObjType(value, OBJ_LIST)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_LIST(value) ((ObjList*)AS_OBJ(value))

/**
 * @brief Enumeration representing object types in the virtual machine.
 *
 * Defines the different types of objects that can be created and managed by the
 * virtual machine.
 *
 * @enum ObjType
 * @value OBJ_BOUND_METHOD Represents a bound method object.
 * @value OBJ_CLASS Represents a class object.
 * @value OBJ_CLOSURE Represents a closure object.
 * @value OBJ_INSTANCE Represents an instance of a class.
 * @value OBJ_FUNCTION Represents a function object.
 * @value OBJ_NATIVE Represents a native function object.
 * @value OBJ_STRING Represents a string object.
 * @value OBJ_UPVALUE Represents an upvalue object.
 */
typedef enum
{
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_INSTANCE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
  OBJ_LIST
} ObjType;

/**
 * @brief Base class for all objects in the virtual machine.
 *
 * Provides common attributes for objects, including type, mark flag, and next
 * pointer for object linking.
 */
class Obj
{
public:
  /**
   * @brief The type of the object.
   */
  ObjType type;
  /**
   * @brief A flag indicating whether the object is marked for garbage
   * collection.
   */
  bool isMarked;
  /**
   * @brief A pointer to the next object in the object list.
   */
  Obj* next;
};

/**
 * @brief fRepresents an upvalue in the virtual machine.
 *
 * An upvalue encapsulates a variable captured from an outer scope. It can hold
 * either a reference to the original variable or a closed value if the variable
 * is no longer accessible.
 */
class ObjUpvalue
{
public:
  /**
   * @brief The base object header.
   */
  Obj obj;

  /**
   * @brief A pointer to the location of the captured variable in the stack.
   */
  Value* location;

  /**
   * @brief The closed value of the upvalue, if the variable is no longer
   * accessible.
   */
  Value closed;

  /**
   * @brief A pointer to the next upvalue in the chain.
   */
  ObjUpvalue* next;
};

/**
 * @brief Represents a string object.
 *
 * Stores the character data, length, and hash of a string.
 */
class ObjString : public Obj
{
public:
  /**
   * @brief The length of the string in characters.
   */
  int length;

  /**
   * @brief A pointer to the characters of the string.
   */
  char* chars;

  /**
   * @brief The hash value of the string.
   */
  uint32_t hash;

#ifdef ENABLE_MTHM
  uint32_t hash2;
#endif
};

class ObjList : public Obj
{
public:
  int count;
  int capacity;
  Value* items;
};

/**
 * @brief Represents a compiled function.
 *
 * Stores information about the function's parameters, upvalues, bytecode, and
 * name.
 */
class ObjFunction : public Obj
{
public:
  /**
   * @brief The number of arguments expected by the function.
   */
  int arity;

  /**
   * @brief The number of upvalues captured by the function.
   */
  int upvalueCount;

  /**
   * @brief The compiled bytecode for the function.
   */
  Chunk chunk;

  /**
   * @brief The name of the function.
   */
  ObjString* name;
};

/**
 * @brief Represents a closure over a function.STRING
 *
 * Stores a reference to the function, its upvalues, and the number of upvalues.
 */
class ObjClosure : public Obj
{
public:
  /**
   * @brief The function associated with the closure.
   */
  ObjFunction* function;

  /**
   * @brief An array of upvalues captured by the closure.
   */
  ObjUpvalue** upvalues;

  /**
   * @brief The number of upvalues in the closure.
   */
  int upvalueCount;
};

/**
 * @brief Represents a class.
 *
 * Stores the class name and a table of its methods.
 */
class ObjClass : public Obj
{
public:
  /**
   * @brief The name of the class.
   */
  ObjString* name;

  /**
   * @brief A table of methods defined for the class.
   */
  Table methods;
};

/**
 * @brief Represents an instance of a class.
 *
 * Stores a reference to the class and a table of instance fields.
 */
class ObjInstance : public Obj
{
public:
  /**
   * @brief The class of the instance.
   */
  ObjClass* klass;

  /**
   * @brief A table of instance fields.
   */
  Table fields;
};

/**
 * @brief Represents a bound method.
 *
 * Stores a reference to the receiver object and the underlying closure.
 */
class ObjBoundMethod : public Obj
{
public:
  /**
   * @brief The receiver object of the bound method.
   */
  Value receiver;

  /**
   * @brief The closure representing the method.
   */
  ObjClosure* method;
};

typedef Value (*NativeFn)(int argCount, Value* args);

/**
 * @brief Represents a native function.
 *
 * Wraps a native function pointer for integration with the virtual machine.
 */
class ObjNative : public Obj
{
public:
  /**
   * @brief The native function pointer.
   */
  NativeFn function;
};

/**
 * @brief Checks if a value is of a specific object type.
 *
 * Determines if the given value is an object and if its type matches the
 * specified object type.
 *
 * @param value The value to check.
 * @param type The object type to compare against.
 * @return `true` if the value is an object of the specified type, `false`
 * otherwise.
 */
static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

/**
 * @brief Creates a new class object.
 *
 * Allocates memory for a new class object and initializes its name.
 *
 * @param name The name of the class.
 * @return A pointer to the newly created class object.
 */
ObjClass* newClass(ObjString* name);

/**
 * @brief Creates a new string object by copying the given characters.
 *
 * Allocates memory for a new string object and copies the given characters into
 * it.
 *
 * @param chars The characters of the string.
 * @param length The length of the string.
 * @return A pointer to the newly created string object.
 */
ObjString* copyString(const char* chars, int length);

/**
 * @brief Creates a new string object by taking ownership of the given
 * characters.
 *
 * Takes ownership of the given character array and creates a new string object.
 *
 * @param chars The characters of the string.
 * @param length The length of the string.
 * @return A pointer to the newly created string object.
 */
ObjString* takeString(char* chars, int length);

/**
 * @brief Creates a new function object.
 *
 * Allocates memory for a new function object and initializes its members.
 *
 * @return A pointer to the newly created function object.
 */
ObjFunction* newFunction();

/**
 * @brief Creates a new native function object.
 *
 * Allocates memory for a new native function object and initializes it with the
 * given native function pointer.
 *
 * @param function The native function to be wrapped.
 * @return A pointer to the newly created native function object.
 */
ObjNative* newNative(NativeFn function);

ObjList* newList();

/**
 * @brief Creates a new closure object.
 *
 * Allocates memory for a new closure object and initializes its members.
 *
 * @param function The function associated with the closure.
 * @return A pointer to the newly created closure object.
 */
ObjClosure* newClosure(ObjFunction* function);

/**
 * @brief Creates a new upvalue object.
 *
 * Allocates memory for a new upvalue object and initializes its members.
 *
 * @param slot A pointer to the local variable to be captured.
 * @return A pointer to the newly created upvalue object.
 */
ObjUpvalue* newUpvalue(Value* slot);

/**
 * @brief Creates a new instance of a class.
 *
 * Allocates memory for a new instance object and initializes its members.
 *
 * @param klass The class to create an instance of.
 * @return A pointer to the newly created instance object.
 */
ObjInstance* newInstance(ObjClass* klass);

/**
 * @brief Creates a new bound method object.
 *
 * Allocates memory for a new bound method object and initializes its members.
 *
 * @param receiver The receiver object of the bound method.
 * @param method The method to be bound.
 * @return A pointer to the newly created bound method object.
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);

/**
 * @brief Prints a human-readable representation of a value.
 *
 * Determines the type of the value and prints an appropriate representation.
 * Handles different value types, including booleans, numbers, nil, and objects.
 *
 * @param value The value to be printed.
 */
void printObject(Value value);

// List functionality
void appendToList(ObjList* list, Value value);

void storeToList(ObjList* list, int index, Value value);

Value indexFromList(ObjList* list, int index);

void deleteFromList(ObjList* list, int index);

bool isValidListIndex(ObjList* list, int index);

// String functionality
void appendToString(ObjString* string, Value value);

void storeToString(ObjString* string, int index, ObjString* item);

Value indexFromString(ObjString* string, int index);

void deleteFromString(ObjString* string, int index);

bool isValidStringIndex(ObjString* string, int index);

#endif