#include <chrono>
#include <thread>

#include "memory.hpp"

#include <stdlib.h>

#include "compiler.hpp"
#include "dispatcher.hpp"
#include "vm.hpp"

#ifdef DEBUG_LOG_GC
#  include <stdio.h>

#  include "debug.hpp"
#endif

#ifdef PARALLEL_MARKING
#  include <omp.h>
#endif

constexpr int GC_HEAP_GROW_FACTOR = 2;

const int MAX_SLEEP_TIME = 45;

#ifndef GENERATIONAL_GC

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
#  ifdef GENERATIONAL_GC
void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  // Move to memory space

  vm->memorySpace.updateNurseryStorage(newSize - oldSize);

  // vm->bytesAllocated += newSize - oldSize;
  // if (newSize > oldSize) {
  //   if (vm->bytesAllocated > vm->nextGC) {
  //     collectGarbage();
  //     // generationalCollectGarbage();
  //   }
  // }

  // Memory space

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  // TODO: allocate object to the new heap
  void* result = realloc(pointer, newSize);

  if (result == NULL)
    exit(1);
  return result;
}

#  else
void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  vm->bytesAllocated += newSize - oldSize;

  if (newSize > oldSize) {
#    ifdef DEBUG_STRESS_GC
    collectGarbage();
#    endif
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

#  endif

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
#  ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#  endif

#  ifdef PARALLEL_MARKING
  object->isMarked = true;
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getopenMPVM();
  vm->lockGreyStack();
  if (vm->grayCapacity < vm->grayCount + 1) {
    vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
    vm->grayStack =
        (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
    if (vm->grayStack == NULL) {
      vm->unlockGreyStack();
      exit(1);
    }
  }
  vm->grayStack[vm->grayCount++] = object;
  vm->unlockGreyStack();
#  else
  object->isMarked = true;
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  if (vm->grayCapacity < vm->grayCount + 1) {
    vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
    vm->grayStack =
        (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
    if (vm->grayStack == NULL) {
      exit(1);
    }
  }
  vm->grayStack[vm->grayCount++] = object;
#  endif
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
#  ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#  endif
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
    case OBJ_FUTURE:
    case OBJ_MUTEX:
    case OBJ_CHANNEL:
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
#  ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#  endif
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
    case OBJ_FUTURE: {
      auto futureObj = (ObjFuture*)object;
      // TODO: call dispatcher to free lock resources if any
      FREE<ObjFuture>(futureObj);
      break;
    }
    case OBJ_MUTEX: {
      auto mutexObj = (ObjMutex*)object;
      // TODO: call lockmanager to free resources if any
      FREE<ObjMutex>(mutexObj);
      break;
    }
    case OBJ_CHANNEL: {
      auto channelObj = (ObjChannel*)object;
      FREE<ObjChannel>(channelObj);
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
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
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
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
#  ifdef PARALLEL_MARKING
  auto index = 0;
  auto end = vm->stackTop - vm->stack;
  // printf("the start \n");
#    pragma omp parallel for
  for (auto index = 0; index < end; index++) {
    dispatcher->setopenMPVM(vm);
    auto slot = vm->stack + index;
    markValue(*slot);
  }
  // printf("the end \n");
  // TODO: fix this with VM
#    pragma omp parallel for
  for (int i = 0; i < vm->frameCount; i++) {
    dispatcher->setopenMPVM(vm);
    markObject((Obj*)vm->frames[i].closure);
  }
  for (auto upvalue = vm->openUpvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }
  vm->globals.markTable();
  markCompilerRoots();
  markObject((Obj*)vm->initString);
#  else
  for (auto slot = vm->stack; slot < vm->stackTop; slot++) {
    markValue(*slot);
  }
  // TODO: fix this with VM
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
#  endif
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
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
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
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
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
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
#  ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm->bytesAllocated;
#  endif
  markRoots();
  traceReferences();
  vm->strings.tableRemoveWhite();
  sweep();
  vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#  ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm->bytesAllocated,
         before,
         vm->bytesAllocated,
         vm->nextGC);
#  endif
}

#else

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  // Move to memory space

  vm->memorySpace.updateNurseryStorage(newSize - oldSize);
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }
  // TODO: allocate object to the new heap
  void* result;
  if (pointer == NULL) {
    result = malloc(newSize);
  } else {
    result = realloc(pointer, newSize);
  }

  if (result == NULL)
    exit(1);
  return result;
}

void markObject(Obj* object, VM* vm)
{
  if (object == NULL)
    return;
  // printObject(OBJ_VAL(object));
  // printf("\n");
  if (object->isMarked)
    return;
#  ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#  endif

  object->isMarked = true;
  if (vm->grayCapacity < vm->grayCount + 1) {
    vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
    vm->grayStack =
        (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);
    if (vm->grayStack == NULL) {
      exit(1);
    }
  }
  vm->grayStack[vm->grayCount++] = object;
}

void markValue(Value value, VM* vm)
{
  if (IS_OBJ(value))
    markObject(AS_OBJ(value), vm);
}

static void markArray(ValueArray* array, VM* vm)
{
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i], vm);
  }
}

static void blackenObject(Obj* object, VM* vm)
{
#  ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#  endif
  switch (object->type) {
    case OBJ_BOUND_METHOD: {
      auto bound = (ObjBoundMethod*)object;
      markValue(bound->receiver, vm);
      markObject((Obj*)bound->method, vm);
      break;
    }
    case OBJ_CLASS: {
      auto klass = (ObjClass*)object;
      markObject((Obj*)klass->name, vm);
      klass->methods.markTable(vm);
      break;
    }
    case OBJ_INSTANCE: {
      auto instance = (ObjInstance*)object;
      markObject((Obj*)instance->klass, vm);
      instance->fields.markTable(vm);
      break;
    }
    case OBJ_CLOSURE: {
      auto closure = (ObjClosure*)object;
      markObject((Obj*)closure->function, vm);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i], vm);
      }
      break;
    }
    case OBJ_FUNCTION: {
      auto function = (ObjFunction*)object;
      markObject((Obj*)function->name, vm);
      markArray(&function->chunk.constants, vm);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed, vm);
      break;
    case OBJ_LIST: {
      ObjList* list = (ObjList*)object;
      for (int i = 0; i < list->count; i++) {
        markValue(list->items[i], vm);
      }
      break;
    }
    case OBJ_FUTURE:
    case OBJ_MUTEX:
    case OBJ_CHANNEL:
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

static void markRememberedSet(VM* vm)
{
  for (auto& x : vm->memorySpace.rememberedSet) {
    markObject(x, vm);
  }
  vm->memorySpace.rememberedSet.clear();
}

static void markRoots(VM* vm)
{
  for (auto slot = vm->stack; slot < vm->stackTop; slot++) {
    markValue(*slot, vm);
  }
  // TODO: fix this with VM
  for (int i = 0; i < vm->frameCount; i++) {
    markObject((Obj*)vm->frames[i].closure, vm);
  }
  for (auto upvalue = vm->openUpvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue, vm);
  }
  vm->globals.markTable(vm);
  markCompilerRoots(vm);
  markObject((Obj*)vm->initString, vm);

  // TODO: mark remembered set
  markRememberedSet(vm);

  // printf("Done Marky Marky \n");
}

void freeObject(Obj* object)
{
  // #ifdef DEBUG_LOG_GC
  // printf("%p free type %d \n", (void*)object, object->type);
  // #endif
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
      // printf("freed string: %s \n", string->chars);
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
    case OBJ_FUTURE: {
      auto futureObj = (ObjFuture*)object;
      // TODO: call dispatcher to free lock resources if any
      FREE<ObjFuture>(futureObj);
      break;
    }
    case OBJ_MUTEX: {
      auto mutexObj = (ObjMutex*)object;
      // TODO: call lockmanager to free resources if any
      FREE<ObjMutex>(mutexObj);
      break;
    }
    case OBJ_CHANNEL: {
      auto channelObj = (ObjChannel*)object;
      FREE<ObjChannel>(channelObj);
      break;
    }
  }
}

void freeObjects()
{
  // printf("Freeing objects \n");
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  auto object = vm->objects;
  while (object != NULL) {
    auto next = object->next;
    freeObject(object);
    object = next;
  }
  free(vm->grayStack);
}

static void traceReferences(VM* vm)
{
  while (vm->grayCount > 0) {
    auto object = vm->grayStack[--vm->grayCount];
    blackenObject(object, vm);
  }
}

void MemoryGeneration::initMG(Generation gen, MemorySpace* ms)
{
  this->head = NULL;
  this->gen = gen;
  this->bytesAllocated = 0;
  this->ms = ms;
  this->head = NULL;
  this->tail = NULL;
  switch (this->gen) {
    case Generation::NURSERY:
      this->nextSweep = 1024 * 1024;
      break;
    case Generation::SURVIVOR:
      this->nextSweep = 1024 * 1024 * 2;
      break;
    case Generation::TENURED:
      this->nextSweep = 1024 * 1024 * 16;
      break;
    default:
      break;
  }
}

void MemoryGeneration::sweep()
{
  // printf("Sweepy Sweepy time \n");
  auto object = this->head;
  Obj* previous = NULL;
  while (object != NULL) {
    // if (this->gen == Generation::SURVIVOR) {
    //   // printf("sweepy sweepy \n");
    // }
    if (object->isMarked) {
      if (this->gen == Generation::SURVIVOR) {
        // printf("markked object \n");
        object->genCount++;
      }
      object->isMarked = false;
      previous = object;
      object = object->next;
    } else {
      if (this->gen == Generation::SURVIVOR) {
        // printf("unmarked object \n");
      }
      auto unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        this->head = object;
      }
      freeObject(unreached);
    }
  }

  this->tail = previous;

  // printf("Done Sweepy Sweepy time \n");
}

void MemoryGeneration::addObject(Obj* newNode)
{
  if (this->tail == NULL) {
    this->tail = newNode;
  }
  newNode->next = this->head;
  this->head = newNode;
}

bool MemoryGeneration::updateStorage(int size)
{
  this->bytesAllocated += size;

  // printf(
  //     "allocated bytes: %d, current capacity: %d, next sweep:  %d, gen:%d
  //     \n", size, this->bytesAllocated, this->nextSweep, this->gen);

  if (size > 0) {
    if (this->bytesAllocated > this->nextSweep) {  // Comment this for testing
      auto sizeBefore = this->bytesAllocated;
      if (this->gen == Generation::NURSERY) {
        this->ms->checkMarkingThreadStateForCollection();
      }
      this->sweep();
      this->increaseCapacity();
      auto sizeAfter = this->bytesAllocated;
      // printf("gen: %d, before: %d, after : %d \n",
      //        this->gen,
      //        sizeBefore,
      //        sizeAfter);
      return true;
    }
  }
  return false;
}

inline int MemoryGeneration::increaseCapacity()
{
  switch (this->gen) {
    case Generation::NURSERY:
      // this->nextSweep = std::min(
      //     static_cast<int64_t>(static_cast<float>(this->nextSweep) * 1.1),
      //     static_cast<int64_t>(1024 * 1024 * 16));
      this->nextSweep =
          static_cast<int64_t>(static_cast<float>(this->nextSweep) * 2);
      break;
    case Generation::SURVIVOR:
      this->nextSweep =
          static_cast<int64_t>(static_cast<float>(this->nextSweep) * 2);
      break;
    case Generation::TENURED:
      this->nextSweep =
          static_cast<int64_t>(static_cast<float>(this->nextSweep) * 16);
      break;
    default:
      break;
  }

  // printf("gen: %d %d next capacity \n", this->gen, this->nextSweep);
  // std::this_thread::sleep_for(std::chrono::seconds(1));

  return this->nextSweep;
}

void MemoryGeneration::unMark()
{
  // printf("Calling unmark here \n");
  auto temp = this->head;
  while (temp != NULL) {
    temp->isMarked = false;
    temp = temp->next;
  }
}

bool MemoryGeneration::resetStorage()
{
  this->bytesAllocated = 0;
  return 0;
}

// Memory Space

void MemorySpace::checkMarkingThreadStateForCollection()
{
  this->startMarking = false;
  // if (this->markState == MarkState::STOP) {
  this->forcedMark();
  // } else {
  // int sleep_time = 1;
  // while (this->markState != MarkState::STOP) {
  // std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
  // sleep_time = std::min(MAX_SLEEP_TIME, sleep_time * 2);
  // }
  // }
}

bool MemorySpace::moveGenerations(MemoryGeneration* A, MemoryGeneration* B)
{
  // printf("Moving generations \n");
  auto res = B->updateStorage(A->bytesAllocated);
  A->bytesAllocated = 0;

  // auto temp = B->head;
  if (B->tail == NULL) {
    B->head = A->head;
    B->tail = A->tail;

    A->head = NULL;
    A->tail = NULL;
    return res;
  }
  // printf("Here kids \n");
  // while (temp->next != NULL) {
  //   temp = temp->next;
  // }
  // temp->next = A.head;

  // TODO: pls check if this works

  B->tail->next = A->head;
  B->tail = A->tail;
  A->head = NULL;
  A->tail = NULL;

  return res;
}

void MemorySpace::doMark()
{
  while (true) {
    if (this->startMarking) {
      // std::cout << "Hello World " << std::endl;
      this->markState = MarkState::RUNNING;
      markRoots(this->vm);
      traceReferences(vm);
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      // std::cout << "Marking Stack" << std::endl;
    }
    this->markState = MarkState::STOP;
  }
}

void MemorySpace::initMS(VM* vm)
{
  this->nursery.initMG(Generation::NURSERY, this);
  this->survivor.initMG(Generation::SURVIVOR, this);
  this->tenured.initMG(Generation::TENURED, this);
  this->markState = MarkState::STOP;
  this->vm = vm;
}

void MemorySpace::startMarkingThread()
{
  this->startMarking = false;
  // memoryThread.emplace_back([this] { this->doMark(); });
}

bool MemorySpace::moveSurvivors()
{
  int space = 0;
  auto survivorHead = this->survivor.head;
  Obj* prev = NULL;
  Obj* temp = new Obj();  // Holds list of objects to add to next list
  auto dummyNode = new Obj();  // dummy node pointer

  auto x = temp, y = dummyNode, z = dummyNode;
  auto count = -1;

  dummyNode->genCount = 0;
  dummyNode->next = survivorHead;

  // while (z != NULL) {
  //   count++;
  //   z = z->next;
  // }

  while (dummyNode != NULL) {
    if (dummyNode->genCount > 2) {
      // printf("Here we go \n");
      space += sizeof(dummyNode);
      temp->next = dummyNode;
      temp = temp->next;
      auto next = dummyNode->next;
      dummyNode->next = NULL;
      prev->next = next;
      dummyNode = next;
    } else {
      prev = dummyNode;
      dummyNode = dummyNode->next;
    }
  }

  // auto a = x, b = y;

  // int count1 = -1, count2 = -1;
  // while (a != NULL) {
  //   printf("%d ", a->genCount);
  //   count1++;
  //   a = a->next;
  // }
  // printf("\n");

  // while (b != NULL) {
  //   printf("%d ", b->genCount);
  //   count2++;
  //   b = b->next;
  // }
  // printf("\n");

  // printf("count1: %d, count2: %d, count: %d \n", count1, count2, count);

  this->survivor.head = y->next;
  this->survivor.tail = prev;

  // printf("allocated bytes: %d, current capacity: %d, next sweep:  %d,
  // gen:%d\n",
  //        space,
  //        this->tenured.bytesAllocated,
  //        this->tenured.nextSweep,
  //        this->tenured.gen);

  auto res = this->tenured.updateStorage(space);
  this->survivor.bytesAllocated -= space;

  if (this->tenured.tail == NULL) {
    this->tenured.head = x->next;
    this->tenured.tail = temp;
  } else {
    this->tenured.tail->next = x->next;
    this->tenured.tail = temp;
  }

  delete x;
  delete y;
  return res;
}

void MemorySpace::updateNurseryStorage(int size)
{
  // printf("pre updating storage \n");
  auto didNurserySweep = this->nursery.updateStorage(size);
  // if (didNurserySweep) {
  //   this->nursery.unMark();
  // }
  if (didNurserySweep) {
    // printf("cleaned new region \n");
    auto didSurvivorsWeep =
        this->moveGenerations(&this->nursery, &this->survivor);

    if (didSurvivorsWeep) {
      // printf("cleaned prev region \n");
      auto didCleanTEnured = this->moveSurvivors();
      // if (didCleanTEnured) {
      // printf("cleaned tenured region \n");
      // }
    }
  }
  // this->tenured.unMark();
  // this->survivor.unMark();
  // }
}

void MemorySpace::addObjectToRememberedSet(Obj* newNode)
{
  // if (this->rememberedSet.find(newNode) != this->rememberedSet.end()) {
  // return;
  // }
  this->rememberedSet.insert(newNode);
}

void MemorySpace::removeObjectFromRememberedSet(Obj* newNode)
{
  if (this->rememberedSet.find(newNode) == this->rememberedSet.end()) {
    return;
  }
  this->rememberedSet.erase(newNode);
}

void MemorySpace::addObjectToNursery(Obj* newNode)
{
  this->nursery.addObject(newNode);
}

void MemorySpace::forcedMark()
{
  // this->doMark();
  // printf("Marking thread \n");
  markRoots(this->vm);
  traceReferences(this->vm);
  this->vm->strings.tableRemoveWhite();
}

void MemorySpace::resumeMarkingThread()
{
  // printf("Resuming marking thread \n");
  this->startMarking = true;
}

void MemorySpace::waitMarkingThread()
{
  // printf("Pausing marking thread \n");
  this->startMarking = false;
}

void MemorySpace::freeMarkingThread()
{
  // printf("Freeing marking thread \n");
  this->startMarking = false;
}

MemorySpace::MemorySpace()
{
  // this->initMS(vm);
}

#endif