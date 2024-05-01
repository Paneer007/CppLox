#include "value.hpp"

#include <stdio.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"

ValueArray::ValueArray()
{
  this->initValueArray();
}

void ValueArray::initValueArray()
{
  this->values = NULL;
  this->capacity = 0;
  this->count = 0;
}

void ValueArray::writeValueArray(Value value)
{
  if (this->capacity < this->count + 1) {
    int oldCapacity = this->capacity;
    this->capacity = GROW_CAPACITY(oldCapacity);
    this->values = GROW_ARRAY(Value, this->values, oldCapacity, this->capacity);
  }
  this->values[this->count] = value;
  this->count++;
}

void ValueArray::freeValueArray()
{
  FREE_ARRAY(Value, this->values, this->capacity);
  this->initValueArray();
}

void printValue(Value value)
{
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
}

bool valuesEqual(Value a, Value b)
{
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
      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->length == bString->length
          && memcmp(aString->chars, bString->chars, aString->length) == 0;
    }
    default:
      return false;  // Unreachable.
  }
}
