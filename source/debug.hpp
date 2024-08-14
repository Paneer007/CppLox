#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.hpp"

/**
 * @brief Disassembles a chunk of bytecode.
 *
 * Prints a human-readable representation of the bytecode instructions to
 stdout.

 * @param chunk The chunk to disassemble.
 * @param name The name of the chunk (for display purposes).
 */
void disassembleChunk(Chunk* chunk, const char* name);

/**
 * @brief Disassembles a single instruction from the given chunk at the
 * specified offset.
 *
 * Prints the disassembled instruction to stdout in a human-readable format.
 *
 * @param chunk The chunk containing the bytecode.
 * @param offset The offset within the chunk to disassemble.
 *
 * @return The offset of the next instruction after the disassembled one.
 */
int disassembleInstruction(Chunk* chunk, int offset);

#endif