#include "value.hpp"

#include <stdio.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"

/**
 * @brief Constructs a new, empty ValueArray.
 *
 * Initializes the ValueArray with default values, ready for use.
 */
ValueArray::ValueArray()
{
  this->initValueArray();
}

/**
 * @brief Initializes an empty ValueArray.
 *
 * Resets the array's internal state, setting the values array to null, capacity
 * to 0, and count to 0.
 */
void ValueArray::initValueArray()
{
  this->values = NULL;
  this->capacity = 0;
  this->count = 0;
}

/**
 * @brief Appends a value to the ValueArray.
 *
 * Resizes the array if necessary to accommodate the new value.
 *
 * @param value The value to be appended.
 */
void ValueArray::writeValueArray(Value value)
{
  if (this->capacity < this->count + 1) {
    int oldCapacity = this->capacity;
    this->capacity = GROW_CAPACITY(oldCapacity);
    this->values = GROW_ARRAY<Value>(this->values, oldCapacity, this->capacity);
  }
  this->values[this->count] = value;
  this->count++;
}

/**
 * @brief Frees the memory allocated for the ValueArray.
 *
 * Deallocates the memory used by the `values` array and resets the array to an
 * empty state.
 */
void ValueArray::freeValueArray()
{
  FREE_ARRAY<Value>(this->values, this->capacity);
  this->initValueArray();
}

/**
 * @brief Prints a human-readable representation of a value.
 *
 * This function determines the type of the value and prints an appropriate
 * representation. It handles boolean, nil, number, and object values.
 *
 * @param value The value to be printed.
 */
void printValue(Value value)
{
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
#else
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      printf("nil");
      break;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(value));
      break;
    case VAL_OBJ:
      printObject(value);
      break;
  }
#endif
}

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
bool valuesEqual(Value a, Value b)
{
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
  if (a.type != b.type)
    return false;
  switch (a.type) {
    case VAL_BOOL:
      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
      return true;
    case VAL_NUMBER:
      return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
      return AS_OBJ(a) == AS_OBJ(b);
    }
    default:
      return false;  // Unreachable.
  }
#endif
}
