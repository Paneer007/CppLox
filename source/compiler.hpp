#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.hpp"

ObjFunction* compile(const char* source);

#endif