#include "memory.hpp"

#include <stdlib.h>

#include "compiler.hpp"
#include "vm.hpp"

#ifdef DEBUG_LOG_GC
#  include <stdio.h>

#  include "debug.hpp"
#endif

constexpr int GC_HEAP_GROW_FACTOR = 2;

/**
 * @brief Reallocates a block of memory.
 *
 * This function is a wrapper around the standard `realloc` function, providing
 * additional functionality for memory management and garbage collection. It
 * updates the allocated memory counter in the VM instance and triggers garbage
 * collection if necessary.
 *
 * @param pointer A pointer to the memory block to be reallocated.
 * @param oldSize The size of the current memory block in bytes.
 * @param newSize The desired size of the new memory block in bytes.
 * @return A pointer to the reallocated memory block, or NULL if allocation
 * fails.
 *
 * **Note:** This function may call `collectGarbage()` if the new size is larger
 * than the old size and certain conditions are met.
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  auto vm = VM::getVM();
  vm->bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
    if (vm->bytesAllocated > vm->nextGC) {
       // collectGarbage();
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

/**
 * @brief Marks an object as reachable for garbage collection.
 *
 * This function marks the given object as reachable by setting its `isMarked`
 * flag to true. It also adds the object to the gray stack for further
 * processing during the garbage collection cycle.
 *
 * If the gray stack capacity is insufficient, it is resized to accommodate the
 * new object.
 *
 * @param object The object to be marked.
 */
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

/**
 * @brief Marks a value as reachable for garbage collection.
 *
 * This function checks if the given value is an object. If it is, it calls
 * `markObject` to mark the object as reachable. This is part of the garbage
 * collection process to identify objects that are still in use.
 *
 * @param value The value to be marked.
 */
void markValue(Value value)
{
  if (IS_OBJ(value))
    markObject(AS_OBJ(value));
}

/**
 * @brief Marks all values within an array as reachable for garbage collection.
 *
 * Iterates over each value in the given array and calls `markValue` on it.
 * This ensures that all objects referenced by the array are considered live
 * during the garbage collection process.
 *
 * @param array The array to be marked.
 */
static void markArray(ValueArray* array)
{
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

/**
 * @brief Marks an object and its dependencies as reachable for garbage
 * collection.
 *
 * This function traverses the object's graph of references, marking all
 * reachable objects. It's a core part of the garbage collection process,
 * determining which objects are still in use.
 *
 * @param object The object to be blackened.
 */
static void blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  switch (object->type) {
    case OBJ_BOUND_METHOD: {
      auto bound = (ObjBoundMethod*)object;
      markValue(bound->receiver);
      markObject((Obj*)bound->method);
      break;
    }
    case OBJ_CLASS: {
      auto klass = (ObjClass*)object;
      markObject((Obj*)klass->name);
      klass->methods.markTable();
      break;
    }
    case OBJ_INSTANCE: {
      auto instance = (ObjInstance*)object;
      markObject((Obj*)instance->klass);
      instance->fields.markTable();
      break;
    }
    case OBJ_CLOSURE: {
      auto closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      auto function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(&function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_LIST: {
      ObjList* list = (ObjList*)object;
      for (int i = 0; i < list->count; i++) {
        markValue(list->items[i]);
      }
      break;
    }
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

/**
 * @brief Frees the memory allocated for an object.
 *
 * This function deallocates the memory associated with an object based on its
 * type. It handles different object types and their specific memory management
 * requirements.
 *
 * @param object The object to be freed.
 */
static void freeObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif
  switch (object->type) {
    case OBJ_BOUND_METHOD:
      FREE<ObjBoundMethod>(object);
      break;
    case OBJ_CLASS: {
      auto klass = (ObjClass*)object;
      klass->methods.freeTable();
      FREE<ObjClass>(object);
      break;
    }
    case OBJ_INSTANCE: {
      auto instance = (ObjInstance*)object;
      instance->fields.freeTable();
      FREE<ObjInstance>(object);
      break;
    }
    case OBJ_CLOSURE: {
      auto closure = (ObjClosure*)object;
      FREE_ARRAY<ObjUpvalue*>(closure->upvalues, closure->upvalueCount);
      FREE<ObjClosure>(object);
      break;
    }
    case OBJ_FUNCTION: {
      auto function = (ObjFunction*)object;
      function->chunk.freeChunk();
      FREE<ObjFunction>(object);
      break;
    }
    case OBJ_NATIVE:
      FREE<ObjNative>(object);
      break;
    case OBJ_STRING: {
      auto string = (ObjString*)object;
      FREE_ARRAY<char>(string->chars, string->length + 1);
      FREE<ObjString>(object);
      break;
    }
    case OBJ_UPVALUE:
      FREE<ObjUpvalue>(object);
      break;
    case OBJ_LIST: {
      ObjList* list = (ObjList*)object;
      FREE_ARRAY<Value>(list->items, list->count);
      FREE<ObjList>(object);
      break;
    }
  }
}

/**
 * @brief Frees all allocated objects in the virtual machine.
 *
 * This function iterates through the linked list of objects, freeing each
 * object's memory using the `freeObject` function. Finally, it deallocates the
 * gray stack used for garbage collection.
 */
void freeObjects()
{
  auto vm = VM::getVM();
  auto object = vm->objects;
  while (object != NULL) {
    auto next = object->next;
    freeObject(object);
    object = next;
  }
  free(vm->grayStack);
}

/**
 * @brief Marks root objects for garbage collection.
 *
 * This function identifies and marks all objects that are considered roots in
 * the virtual machine. Roots are objects that are directly accessible by the
 * program and should not be garbage collected. These include objects on the
 * stack, in closures, upvalues, globals, and other critical areas.
 */
static void markRoots()
{
  auto vm = VM::getVM();
  for (auto slot = vm->stack; slot < vm->stackTop; slot++) {
    markValue(*slot);
  }
  for (int i = 0; i < vm->frameCount; i++) {
    markObject((Obj*)vm->frames[i].closure);
  }
  for (auto upvalue = vm->openUpvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }
  vm->globals.markTable();
  markCompilerRoots();
  markObject((Obj*)vm->initString);
}

/**
 * @brief Traces object references for garbage collection.
 *
 * This function processes the gray stack to identify and mark reachable
 * objects. It iterates through the gray stack, blackening each object and its
 * references. This process continues until the gray stack is empty, indicating
 * that all reachable objects have been marked.
 */
static void traceReferences()
{
  auto vm = VM::getVM();
  while (vm->grayCount > 0) {
    auto object = vm->grayStack[--vm->grayCount];
    blackenObject(object);
  }
}

/**
 * @brief Sweeps the object list and frees unreachable objects.
 *
 * This function iterates through the list of objects, removing and freeing
 * those that are not marked as reachable. Reachable objects have their
 * `isMarked` flag reset for the next garbage collection cycle.
 *
 * The object list is maintained as a linked list, and this function updates the
 * head of the list if necessary.
 */
static void sweep()
{
  auto vm = VM::getVM();
  Obj* previous = NULL;
  auto object = vm->objects;
  while (object != NULL) {
    if (object->isMarked) {
      object->isMarked = false;
      previous = object;
      object = object->next;
    } else {
      auto unreached = object;
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

/**
 * @brief Performs garbage collection on the virtual machine.
 *
 * This function initiates the garbage collection process to reclaim memory
 * occupied by unreachable objects. It involves marking reachable objects,
 * tracing references, removing white objects from the string table, and
 * sweeping the object list to free unreachable objects.
 *
 * The function also calculates and updates the next garbage collection
 * threshold based on the current memory usage.
 */
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