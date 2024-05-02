#ifndef clox_object_h
#define clox_object_h

#include "common.hpp"
#include "value.hpp"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum
{
  OBJ_STRING,
} ObjType;

class Obj
{
public:
  ObjType type;
  Obj* next;
};

class ObjString : public Obj
{
public:
  int length;
  char* chars;
  uint32_t hash;
};

static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString* copyString(const char* chars, int length);
ObjString* takeString(char* chars, int length);

void printObject(Value value);

#endif