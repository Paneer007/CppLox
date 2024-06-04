#include "memory.hpp"

#include <stdlib.h>

#include "compiler.hpp"
#include "vm.hpp"

#ifdef DEBUG_LOG_GC
#  include <stdio.h>

#  include "debug.hpp"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  auto vm = VM::getVM();
  vm->bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    if (vm->bytesAllocated > vm->nextGC) {
      collectGarbage();
    }
  }
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL)
    exit(1);
  return result;
}

void markObject(Obj* object)
{
  if (object == NULL)
    return;
  if (object->isMarked)
    return;
#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  object->isMarked = true;
  auto vm = VM::getVM();
  if (vm->grayCapacity < vm->grayCount + 1) {
    vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
    vm->grayStack =
        (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
    if (vm->grayStack == NULL)
      exit(1);
  }

  vm->grayStack[vm->grayCount++] = object;
}

void markValue(Value value)
{
  if (IS_OBJ(value))
    markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array)
{
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

static void blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  switch (object->type) {
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      markValue(bound->receiver);
      markObject((Obj*)bound->method);
      break;
    }
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      markObject((Obj*)klass->name);
      klass->methods.markTable();
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      markObject((Obj*)instance->klass);
      instance->fields.markTable();
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(&function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

static void freeObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif
  switch (object->type) {
    case OBJ_BOUND_METHOD:
      FREE(ObjBoundMethod, object);
      break;
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      klass->methods.freeTable();
      FREE(ObjClass, object);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      instance->fields.freeTable();
      FREE(ObjInstance, object);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      function->chunk.freeChunk();
      FREE(ObjFunction, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
  }
}

void freeObjects()
{
  VM* vm = VM::getVM();
  Obj* object = vm->objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
  free(vm->grayStack);
}

static void markRoots()
{
  auto vm = VM::getVM();
  for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
    markValue(*slot);
  }
  for (int i = 0; i < vm->frameCount; i++) {
    markObject((Obj*)vm->frames[i].closure);
  }
  for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL;
       upvalue = upvalue->next)
  {
    markObject((Obj*)upvalue);
  }
  vm->globals.markTable();
  markCompilerRoots();
  markObject((Obj*)vm->initString);
}

static void traceReferences()
{
  VM* vm = VM::getVM();
  while (vm->grayCount > 0) {
    Obj* object = vm->grayStack[--vm->grayCount];
    blackenObject(object);
  }
}

static void sweep()
{
  auto vm = VM::getVM();
  Obj* previous = NULL;
  Obj* object = vm->objects;
  while (object != NULL) {
    if (object->isMarked) {
      object->isMarked = false;
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm->objects = object;
      }

      freeObject(unreached);
    }
  }
}

void collectGarbage()
{
  auto vm = VM::getVM();
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm->bytesAllocated;
#endif
  markRoots();
  traceReferences();
  vm->strings.tableRemoveWhite();
  sweep();
  vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm->bytesAllocated,
         before,
         vm->bytesAllocated,
         vm->nextGC);
#endif
}