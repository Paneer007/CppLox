#include "object.hpp"

#include <stdio.h>
#include <string.h>

#include "memory.hpp"
#include "table.hpp"
#include "value.hpp"
#include "vm.hpp"

#define ALLOCATE_OBJ(type, objectType) \
  (type*)allocateObject(sizeof(type), objectType)

static uint32_t hashString(const char* key, int length)
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

static void printFunction(ObjFunction* function)
{
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

static Obj* allocateObject(size_t size, ObjType type)
{
  VM* vm = VM::getVM();
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;
  object->next = vm->objects;
  vm->objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method)
{
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjClass* newClass(ObjString* name)
{
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  klass->methods.initTable();
  return klass;
}

ObjFunction* newFunction()
{
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  function->chunk.initChunk();
  return function;
}

ObjNative* newNative(NativeFn function)
{
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash)
{
  VM* vm = VM::getVM();
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  vm->push(OBJ_VAL(string));
  vm->strings.tableSet(string, NIL_VAL);
  vm->pop();
  return string;
}

ObjString* copyString(const char* chars, int length)
{
  uint32_t hash = hashString(chars, length);
  VM* vm = VM::getVM();
  ObjString* interned = vm->strings.tableFindString(chars, length, hash);
  if (interned != NULL)
    return interned;
  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

ObjString* takeString(char* chars, int length)
{
  uint32_t hash = hashString(chars, length);
  return allocateString(chars, length, hash);
}

void printObject(Value value)
{
  switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
  }
}

ObjUpvalue* newUpvalue(Value* slot)
{
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

ObjClosure* newClosure(ObjFunction* function)
{
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjInstance* newInstance(ObjClass* klass)
{
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  instance->fields.initTable();
  return instance;
}
