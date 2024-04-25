#ifndef clox_value_h
#define clox_value_h

#include "common.hpp"

typedef double Value;

class ValueArray
{
public:
  int capacity;
  int count;
  Value* values;

  ValueArray();

  void initValueArray();
  void writeValueArray(Value value);
  void freeValueArray();
};

void printValue(Value value);

#endif