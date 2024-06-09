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

typedef enum
{
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ
} ValueType;

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