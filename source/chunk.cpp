#include "chunk.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "memory.hpp"
#include "vm.hpp"

/**
 * @brief Construct a new Chunk:: Chunk object
 */
Chunk::Chunk()
{
  this->initChunk();
}

/**
 * @brief Appends a bytecode element to the end of an array
 *
 * @param byte byte code character
 * @param line line number of bytecode
 */
void Chunk::writeChunk(uint8_t byte, int line)
{
  // Resizes internal arrays if necessary. Checks if current capacity is
  // sufficient for next element.
  // If not, doubles capacity, reallocates 'code' and 'lines' arrays to new size
  // using 'GROW_ARRAY' function.
  if (this->capacity < this->count + 1) {
    int old_capacity = this->capacity;
    this->capacity = GROW_CAPACITY(old_capacity);
    this->code = GROW_ARRAY<uint8_t>(this->code, old_capacity, this->capacity);
    this->lines = GROW_ARRAY<int>(this->lines, old_capacity, this->capacity);
  }

  // Appends the bytecode and line number to the array
  // Increments the count of elements in the array
  this->code[this->count] = byte;
  this->lines[this->count] = line;
  this->count++;
}

/**
 * @brief Initialises the Chunk
 */
void Chunk::initChunk()
{
  // Sets count and capacity to 0, code and lines to NULL,
  // and initializes the constants array.
  this->count = 0;
  this->capacity = 0;
  this->code = NULL;
  this->constants.initValueArray();
  this->lines = NULL;
}

/**
 * @brief Free all the properties associated with chunk
 */
void Chunk::freeChunk()
{
  FREE_ARRAY<uint8_t>(this->code, this->capacity);
  FREE_ARRAY<int>(this->lines, this->capacity);
  this->constants.freeValueArray();
  this->initChunk();
}

/**
 * @brief Add a value to the chunk array and return index
 * 
 * @param value Value to be appended to the chunk array
 * @return int index of element 
 */
int Chunk::addConstant(Value value)
{
  auto vm = VM::getVM();
  vm->push(value);
  this->constants.writeValueArray(value);
  vm->pop();
  return this->constants.count - 1;
}