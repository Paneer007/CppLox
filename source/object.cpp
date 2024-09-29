#include "object.hpp"

#include <stdio.h>
#include <string.h>

#include "dispatcher.hpp"
#include "memory.hpp"
#include "table.hpp"
#include "value.hpp"
#include "vm.hpp"

/**
 * @brief Calculates a hash value for a given string.
 *
 * This function computes a 32-bit hash value for the specified string using a
 * simple hash function. The hash function is based on XORing each character
 * with the current hash value and multiplying by a prime number.
 *
 * @param key The string to hash.
 * @param length The length of the string.
 * @return The calculated hash value.
 */
static uint32_t hashString(const char* key, int length)
{
#ifdef ENABLE_MP
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
#else
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
#endif
}

#ifdef ENABLE_MP
static inline uint32_t murmur_32_scramble(uint32_t k)
{
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}
static uint32_t hash2ndString(const char* key,
                              int len,
                              uint32_t seed = 0x9747b28c)

{
  uint32_t h = seed;
  uint32_t k;
  for (size_t i = len >> 2; i; i--) {
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  k = 0;
  for (size_t i = len & 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
  h ^= murmur_32_scramble(k);
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  if (h % 2 == 0) {
    h += 1;
  }

  // return h;
  return 1;
}
#endif

/**
 * @brief Prints a representation of a function object.
 *
 * This function prints a formatted string representing the function object.
 * If the function has a name, it prints the name enclosed in angle brackets.
 * Otherwise, it prints "<script>" to indicate an anonymous function.
 *
 * @param function The function object to print.
 */
static void printFunction(ObjFunction* function)
{
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

/**
 * @brief Allocates memory for a new object.
 *
 * This function allocates memory for a new object of the specified size and
 * type. The allocated object is added to the beginning of the object list
 * managed by the VM.
 *
 * @param size The size of the object in bytes.
 * @param type The type of the object.
 * @return A pointer to the newly allocated object.
 */
static Obj* allocateObject(size_t size, ObjType type)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  auto object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;
  object->next = vm->objects;
  vm->objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

/**
 * @brief Allocates memory for an object of a specific type.
 *
 * This template function allocates memory for an object of type `T` and assigns
 * it the specified object type.
 *
 * @tparam T The type of the object to allocate.
 * @param type The object type to assign to the allocated object.
 * @return A pointer to the newly allocated object.
 */
template<typename T>
T* ALLOCATE_OBJ(ObjType x)
{
  return (T*)allocateObject(sizeof(T), x);
}

/**
 * @brief Creates a new bound method object.
 *
 * This function allocates a new `ObjBoundMethod` object, initializes its
 * `receiver` and `method` fields, and returns a pointer to the newly created
 * object.
 *
 * @param receiver The receiver object of the bound method.
 * @param method The method to be bound.
 * @return A pointer to the newly created bound method object.
 */
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method)
{
  auto bound = ALLOCATE_OBJ<ObjBoundMethod>(OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

/**
 * @brief Creates a new class object.
 *
 * This function allocates a new `ObjClass` object, initializes its name, and
 * creates an empty method table.
 *
 * @param name The name of the class.
 * @return A pointer to the newly created class object.
 */
ObjClass* newClass(ObjString* name)
{
  auto klass = ALLOCATE_OBJ<ObjClass>(OBJ_CLASS);
  klass->name = name;
  klass->methods.initTable();
  return klass;
}

/**
 * @brief Creates a new function object.
 *
 * This function allocates a new `ObjFunction` object, initializes its
 * properties, and creates an empty chunk for its code.
 *
 * @return A pointer to the newly created function object.
 */
ObjFunction* newFunction()
{
  auto function = ALLOCATE_OBJ<ObjFunction>(OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  function->chunk.initChunk();
  return function;
}

/**
 * @brief Creates a new native function object.
 *
 * This function allocates a new `ObjNative` object and initializes its
 * `function` field to the given native function pointer.
 *
 * @param function The native function to be wrapped.
 * @return A pointer to the newly created native function object.
 */
ObjNative* newNative(NativeFn function)
{
  auto native = ALLOCATE_OBJ<ObjNative>(OBJ_NATIVE);
  native->function = function;
  return native;
}

/**
 * @brief Allocates a new string object.
 *
 * This function creates a new string object by allocating memory, initializing
 * its properties, and adding it to the string table. The string is also pushed
 * onto the VM's value stack for subsequent operations.
 *
 * @param chars The character array representing the string.
 * @param length The length of the string.
 * @param hash The pre-calculated hash value of the string.
 * @return A pointer to the newly created string object.
 */
static ObjString* allocateString(char* chars,
                                 int length,
                                 uint32_t hash,
                                 uint32_t hash2 = 0)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();
  auto string = ALLOCATE_OBJ<ObjString>(OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

#ifdef ENABLE_MP
  string->hash2 = hash2;
#endif

  vm->push(OBJ_VAL(string));
  vm->strings.tableSet(string, NIL_VAL);
  vm->pop();
  return string;
}

/**
 * @brief Creates a new string object, interning it if possible.
 *
 * This function creates a new string object by copying the given characters.
 * It first checks if the string is already interned in the string table.
 * If found, the existing interned string is returned.
 * Otherwise, a new string object is allocated and added to the string table.
 *
 * @param chars The characters of the string to copy.
 * @param length The length of the string.
 * @return A pointer to the string object, either newly created or interned.
 */
ObjString* copyString(const char* chars, int length)
{
  uint32_t hash = hashString(chars, length);

#ifdef ENABLE_MP
  auto hash2 = hash2ndString(chars, length);
#endif

  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVM();

#ifdef ENABLE_MP
  auto interned = vm->strings.tableFindString(chars, length, hash, hash2);
#else
  auto interned = vm->strings.tableFindString(chars, length, hash, 0);
#endif

  if (interned != NULL)
    return interned;
  auto heapChars = ALLOCATE<char>(length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
#ifdef ENABLE_MP
  return allocateString(heapChars, length, hash, hash2);
#else
  return allocateString(heapChars, length, hash);
#endif
}

/**
 * @brief Creates a new string object, taking ownership of the provided
 * character array.
 *
 * This function creates a new string object using the given character array and
 * its length. It's assumed that the caller relinquishes ownership of the
 * character array. The function calculates the hash of the string, checks for
 * string interning, and allocates a new string object if necessary.
 *
 * **Note:** The caller must ensure that the `chars` array remains valid until
 * the returned string object is no longer used.
 *
 * @param chars The character array to be owned by the string object.
 * @param length The length of the character array.
 * @return A pointer to the newly created string object.
 */
ObjString* takeString(char* chars, int length)
{
  auto hash = hashString(chars, length);
#ifdef ENABLE_MP
  auto hash2 = hash2ndString(chars, length);
  return allocateString(chars, length, hash, hash2);
#else
  return allocateString(chars, length, hash);
#endif
}

/**
 * @brief Prints a human-readable representation of a value.
 *
 * This function determines the type of the value and prints an appropriate
 * representation. It handles various object types, including bound methods,
 * classes, closures, functions, strings, native functions, upvalues, and
 * instances.
 *
 * @param value The value to be printed.
 */
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
    case OBJ_FUTURE:
      printf("<future obj>");
      break;
    case OBJ_LIST:
      printf("[");
      for (int i = 0; i < AS_LIST(value)->count; i++) {
        printValue(AS_LIST(value)->items[i]);
        if (i != AS_LIST(value)->count - 1) {
          printf(",");
        }
      }
      printf("]");
      break;
  }
}

/**
 * @brief Creates a new upvalue object.
 *
 * This function allocates a new `ObjUpvalue` object, initializes its fields,
 * and returns a pointer to the newly created object. Upvalues are used to
 * capture variables from enclosing scopes.
 *
 * @param slot A pointer to the value slot in the stack.
 * @return A pointer to the newly created upvalue object.
 */
ObjUpvalue* newUpvalue(Value* slot)
{
  auto upvalue = ALLOCATE_OBJ<ObjUpvalue>(OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

/**
 * @brief Creates a new closure object.
 *
 * This function allocates a new `ObjClosure` object, initializes its `function`
 * and `upvalues` fields, and returns a pointer to the newly created closure.
 *
 * @param function The function associated with the closure.
 * @return A pointer to the newly created closure object.
 */
ObjClosure* newClosure(ObjFunction* function)
{
  auto upvalues = ALLOCATE<ObjUpvalue*>(function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  auto closure = ALLOCATE_OBJ<ObjClosure>(OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFuture* newFuture(int vm_id)
{
  auto future = ALLOCATE_OBJ<ObjFuture>(OBJ_FUTURE);
  future->vm_id = vm_id;
  return future;
}

/**
 * @brief Creates a new instance of a class.
 *
 * This function allocates a new `ObjInstance` object, initializes its `klass`
 * field, and creates an empty field table for the instance.
 *
 * @param klass The class to create an instance of.
 * @return A pointer to the newly created instance object.
 */
ObjInstance* newInstance(ObjClass* klass)
{
  auto instance = ALLOCATE_OBJ<ObjInstance>(OBJ_INSTANCE);
  instance->klass = klass;
  instance->fields.initTable();
  return instance;
}

ObjList* newList()
{
  ObjList* list = ALLOCATE_OBJ<ObjList>(OBJ_LIST);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
  return list;
}

void appendToList(ObjList* list, Value value)
{
  if (list->capacity < list->count + 1) {
    int oldCapacity = list->capacity;
    list->capacity = GROW_CAPACITY(oldCapacity);
    list->items = GROW_ARRAY<Value>(list->items, oldCapacity, list->capacity);
  }
  list->items[list->count] = value;
  list->count++;
  return;
}

void storeToList(ObjList* list, int index, Value value)
{
  list->items[index] = value;
}
//
Value indexFromList(ObjList* list, int index)
{
  return list->items[index];
}

void deleteFromList(ObjList* list, int index)
{
  for (int i = index; i < list->count - 1; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->items[list->count - 1] = NIL_VAL;
  list->count--;
}

bool isValidListIndex(ObjList* list, int index)
{
  if (index < 0 || index > list->count - 1) {
    return false;
  }
  return true;
}

// TODO: String Operator

void appendToString(ObjString* string, Value value) {}

void storeToString(ObjString* string, int index, ObjString* item)
{
  string->chars[index] = item->chars[0];
}

Value indexFromString(ObjString* string, int index)
{
  auto val = string->chars[index];
  return OBJ_VAL(copyString(&val, 1));
}

void deleteFromString(ObjString* string, int index) {}

bool isValidStringIndex(ObjString* string, int index)
{
  if (index < 0 || index > string->length - 1) {
    return false;
  }
  return true;
}