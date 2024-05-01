#ifndef clox_value_h
#define clox_value_h

#include "common.hpp"

typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ
} ValueType;

class Obj;
class ObjString;

class Value
{
public:
  ValueType type;
  union
  {
    bool boolean;
    double number;
    Obj* obj;
  } as;
};

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define BOOL_VAL(value) ((Value) {VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value) {VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value) {VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value) {VAL_OBJ, {.obj = (Obj*)object}})

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
bool valuesEqual(Value a, Value b);

#endif