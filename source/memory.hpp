#ifndef clox_memory_h
#define clox_memory_h

#include "common.hpp"
#include "object.hpp"

/**
 * @brief Reallocates a block of memory.
 *
 * This function changes the size of a previously allocated block of memory.
 *
 * @param pointer A pointer to the memory block to be reallocated.
 * @param oldSize The size of the current memory block in bytes.
 * @param newSize The desired size of the new memory block in bytes.
 * @return A pointer to the reallocated memory block, or NULL if allocation
 * fails.
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

/**
 * @brief Marks an object as reachable for garbage collection.
 *
 * This function indicates that the given object is still in use and should not
 * be freed during garbage collection.
 *
 * @param object A pointer to the object to be marked.
 */
void markObject(Obj* object);

/**
 * @brief Marks a value as reachable for garbage collection.
 *
 * This function marks a value as reachable if it represents an object.
 *
 * @param value The value to be marked.
 */
void markValue(Value value);

/**
 * @brief Performs garbage collection.
 *
 * This function reclaims memory occupied by unreachable objects.
 */
void collectGarbage();

/**
 * @brief Frees all allocated objects.
 *
 * This function deallocates the memory used by all objects.
 */
void freeObjects();

/**
 * @brief Marks compiler-related objects as roots for garbage collection.
 *
 * This function prevents objects used by the compiler from being freed during
 * garbage collection.
 */
void markCompilerRoots();

/**
 * @brief Calculates the new capacity for a growing data structure.
 *
 * This inline function determines the new capacity for a data structure that
 * needs to be expanded. It doubles the current capacity if it's greater than or
 * equal to 8, otherwise returns 8.
 *
 * @param capacity The current capacity of the data structure.
 * @return The new capacity for the data structure.
 */
inline int GROW_CAPACITY(int capacity)
{
  return (capacity) < 8 ? 8 : (capacity)*2;
}

/**
 * @brief Allocates a block of memory for an array of elements.
 *
 * This template function allocates memory for an array of `count` elements of
 * type `T`. It uses the `reallocate` function to allocate the required memory.
 *
 * @tparam T The type of elements in the array.
 * @param count The number of elements to allocate.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
template<typename T>
inline T* ALLOCATE(int count)
{
  return (T*)reallocate(NULL, 0, sizeof(T) * (count));
}

/**
 * @brief Resizes an existing array to a new size.
 *
 * This template function reallocates an existing array to a new size.
 * It preserves the existing elements and allocates additional space for new
 * elements.
 *
 * @tparam T The type of elements in the array.
 * @param pointer A pointer to the existing array.
 * @param oldCount The number of elements in the current array.
 * @param newCount The desired number of elements in the new array.
 * @return A pointer to the resized array, or NULL if allocation fails.
 */
template<typename T>
inline T* GROW_ARRAY(void* pointer, int oldCount, int newCount)
{
  return (T*)reallocate(
      pointer, sizeof(T) * (oldCount), sizeof(T) * (newCount));
}

/**
 * @brief Frees the memory allocated for an array.
 *
 * This template function deallocates the memory block previously allocated for
 * an array.
 *
 * @tparam T The type of elements in the array.
 * @param pointer A pointer to the memory block to be freed.
 * @param oldCount The number of elements in the array.
 * @return A null pointer.
 */
template<typename T>
inline void* FREE_ARRAY(void* pointer, int oldCount)
{
  return reallocate(pointer, sizeof(T) * (oldCount), 0);
}

/**
 * @brief Frees a block of memory allocated for a single object.
 *
 * This template function deallocates the memory block previously allocated for
 * a single object of type `T`.
 *
 * @tparam T The type of the object.
 * @param pointer A pointer to the memory block to be freed.
 * @return A null pointer.
 */
template<typename T>
inline void* FREE(void* pointer)
{
  return reallocate(pointer, sizeof(T), 0);
}

#endif