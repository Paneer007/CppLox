#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.hpp"

bool compile(const char* source, Chunk* chunk);

#endif