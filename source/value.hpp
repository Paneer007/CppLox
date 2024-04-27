#ifndef clox_value_h
#define clox_value_h

#include "common.hpp"

typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} ValueType;

class Value
{
public:
  ValueType type;
  union
  {
    bool boolean;
    double number;
  } as;
};

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value) {VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value) {VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value) {VAL_NUMBER, {.number = value}})

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