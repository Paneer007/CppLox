#ifndef clox_value_h
#define clox_value_h

#include <string.h>

#include "common.hpp"

class Obj;
class ObjString;

#ifdef NAN_BOXING

#  define QNAN ((uint64_t)0x7ffc000000000000)
#  define SIGN_BIT ((uint64_t)0x8000000000000000)

#  define TAG_NIL 1  // 01.
#  define TAG_FALSE 2  // 10.
#  define TAG_TRUE 3  // 11.

typedef uint64_t Value;

#  define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#  define IS_NIL(value) ((value) == NIL_VAL)
#  define IS_NUMBER(value) (((value)&QNAN) != QNAN)
#  define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#  define AS_BOOL(value) ((value) == TRUE_VAL)
#  define AS_NUMBER(value) valueToNum(value)
#  define AS_OBJ(value) ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#  define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#  define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#  define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#  define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#  define NUMBER_VAL(num) numToValue(num)
#  define OBJ_VAL(obj) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline Value numToValue(double num)
{
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

static inline double valueToNum(Value value)
{
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

#else

/**
 * @brief Enumeration representing the type of a value.
 *
 * This enum defines the possible types a value can hold within the system.
 *
 * @enum ValueType
 * @value VAL_BOOL Represents a boolean value (true or false).
 * @value VAL_NIL Represents a null or nil value.
 * @value VAL_NUMBER Represents a numeric value.
 * @value VAL_OBJ Represents an object value.
 */
typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ
} ValueType;

/**
 * @brief Represents a value in the virtual machine.
 *
 * This union stores a value of different types: boolean, number, or object.
 * The `type` member indicates the actual type of the stored value.
 */
class Value
{
public:
  /**
   * @brief The type of the value.
   */
  ValueType type;

  /**
   * @brief Union to hold the value based on its type.
   */
  union
  {
    bool boolean;
    double number;
    Obj* obj;
  } as;
};

#  define IS_BOOL(value) ((value).type == VAL_BOOL)
#  define IS_NIL(value) ((value).type == VAL_NIL)
#  define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#  define IS_OBJ(value) ((value).type == VAL_OBJ)

#  define AS_BOOL(value) ((value).as.boolean)
#  define AS_NUMBER(value) ((value).as.number)
#  define AS_OBJ(value) ((value).as.obj)

#  define BOOL_VAL(value) ((Value) {VAL_BOOL, {.boolean = value}})
#  define NIL_VAL ((Value) {VAL_NIL, {.number = 0}})
#  define NUMBER_VAL(value) ((Value) {VAL_NUMBER, {.number = value}})
#  define OBJ_VAL(object) ((Value) {VAL_OBJ, {.obj = (Obj*)object}})

#endif

/**
 * @brief Represents a dynamic array of values.
 *
 * This class provides a flexible way to store a growing collection of values.
 */
class ValueArray
{
public:
  /**
   * @brief The maximum capacity of the array.
   */
  int capacity;

  /**
   * @brief The number of elements currently stored in the array.
   */
  int count;

  /**
   * @brief The array of values.
   */
  Value* values;

  /**
   * @brief Constructs a new, empty ValueArray.
   */
  ValueArray();

  /**
   * @brief Initializes an empty ValueArray.
   *
   */
  void initValueArray();

  /**
   * @brief  Appends a value to the ValueArray.
   *
   * @param value The value to be appended.
   */
  void writeValueArray(Value value);

  /**
   * @brief Frees the memory allocated for the ValueArray.
   *
   * Deallocates the memory used by the `values` array and resets the array to
   * an empty state.
   */
  void freeValueArray();
};

/**
 * @brief Prints a human-readable representation of a value.
 *
 * This function determines the type of the value and prints an appropriate
 * representation. It handles boolean, nil, number, and object values.
 *
 * @param value The value to be printed.
 */
void printValue(Value value);

/**
 * @brief Compares two values for equality.
 *
 * Determines if two values are equal based on their types and values.
 * Handles different value types, including numbers, booleans, nil, and objects.
 *
 * @param a The first value to compare.
 * @param b The second value to compare.
 * @return `true` if the values are equal, `false` otherwise.
 */
bool valuesEqual(Value a, Value b);

#endif