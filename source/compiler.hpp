#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.hpp"

/**
 * Compiles the given source code into a function object.

 * Creates a scanner, compiler, and parser to process the source code and
 generate bytecode.

 * @param source The source code to be compiled.
 * @return A pointer to the compiled function object, or NULL on error.
 */
ObjFunction* compile(const char* source);

#endif