#include "value.hpp"

#include <stdio.h>

#include "memory.hpp"

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
  }
}