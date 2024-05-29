#include "chunk.hpp"
#include "memory.hpp"

#include <stdio.h>
#include <stdlib.h>

Chunk::Chunk()
{
  this->initChunk();
}

void Chunk::writeChunk(uint8_t byte, int line)
{
  if (this->capacity < this->count + 1) {
    int old_capacity = this->capacity;
    this->capacity = GROW_CAPACITY(old_capacity);
    this->code = GROW_ARRAY(uint8_t, this->code, old_capacity, this->capacity);
    this->lines = GROW_ARRAY(int, this->lines, old_capacity, this->capacity);
  }

  this->code[this->count] = byte;
  this->lines[this->count] = line;
  this->count++;
}

void Chunk::initChunk()
{
  this->count = 0;
  this->capacity = 0;
  this->code = NULL;
  this->constants.initValueArray();
  this->lines = NULL;
}

void Chunk::freeChunk()
{
  FREE_ARRAY(uint8_t, this->code, this->capacity);
  FREE_ARRAY(int, this->lines, this->capacity);
  this->constants.freeValueArray();
  this->initChunk();
}

int Chunk::addConstant(Value value)
{
  this->constants.writeValueArray(value);
  return this->constants.count - 1;
}