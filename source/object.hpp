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

#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))

typedef enum
{
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_INSTANCE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE
} ObjType;

class Obj
{
public:
  ObjType type;
  bool isMarked;
  Obj* next;
};

class ObjUpvalue
{
public:
  Obj obj;
  Value* location;
  Value closed;
  ObjUpvalue* next;
};

class ObjString : public Obj
{
public:
  int length;
  char* chars;
  uint32_t hash;
};

class ObjFunction
{
public:
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;
};

class ObjClosure
{
public:
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
};

class ObjClass
{
public:
  Obj obj;
  ObjString* name;
};

typedef struct
{
  Obj obj;
  ObjClass* klass;
  Table fields;
} ObjInstance;

typedef Value (*NativeFn)(int argCount, Value* args);

class ObjNative
{
public:
  Obj obj;
  NativeFn function;
};

static inline bool isObjType(Value value, ObjType type)
{
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjClass* newClass(ObjString* name);
ObjString* copyString(const char* chars, int length);
ObjString* takeString(char* chars, int length);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjClosure* newClosure(ObjFunction* function);
ObjUpvalue* newUpvalue(Value* slot);
ObjInstance* newInstance(ObjClass* klass);

void printObject(Value value);

#endif