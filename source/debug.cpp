#include "debug.hpp"

#include <stdio.h>

#include "object.hpp"
#include "value.hpp"

/**
 * @brief Prints a simple instruction and returns the next offset.

 * This function is primarily for debugging purposes.

 * @param name The name of the instruction.
 * @param offset The current offset in the code.
 * @return The next offset in the code.
 */
static int simpleInstruction(const char* name, int offset)
{
  printf("%s\n", name);
  return offset + 1;
}

/**
 * @brief Prints a jump instruction with its target address.

 * This function is primarily for debugging purposes.

 * @param name The name of the instruction.
 * @param sign The sign of the jump offset (-1 for backward, 1 for forward).
 * @param chunk The chunk containing the instruction.
 * @param offset The offset of the jump instruction in the chunk.
 * @return The next offset in the chunk.
 */
static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset)
{
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

/**
 * @brief Disassembles a chunk of bytecode.
 *
 * Prints a human-readable representation of the bytecode instructions to
 stdout.

 * @param chunk The chunk to disassemble.
 * @param name The name of the chunk (for display purposes).
 */
void disassembleChunk(Chunk* chunk, const char* name)
{
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

/**
 * @brief Disassembles a constant instruction.

 * Prints the instruction name, constant index, and the corresponding value.

 * @param name The name of the instruction.
 * @param chunk The chunk containing the instruction.
 * @param offset The offset of the instruction in the chunk.
 * @return The next offset in the chunk.
 */
static int constantInstruction(const char* name, Chunk* chunk, int offset)
{
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

/**
 * @brief Disassembles a byte instruction.

 * Prints the instruction name and the operand value.

 * @param name The name of the instruction.
 * @param chunk The chunk containing the instruction.
 * @param offset The offset of the instruction in the chunk.
 * @return The next offset in the chunk.
 */
static int byteInstruction(const char* name, Chunk* chunk, int offset)
{
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

/**
 * @brief Disassembles an invoke instruction.

 * Prints the instruction name, argument count, and the invoked function's name.

 * @param name The name of the instruction.
 * @param chunk The chunk containing the instruction.
 * @param offset The offset of the instruction in the chunk.
 * @return The next offset in the chunk.
 */
static int invokeInstruction(const char* name, Chunk* chunk, int offset)
{
  uint8_t constant = chunk->code[offset + 1];
  uint8_t argCount = chunk->code[offset + 2];
  printf("%-16s (%d args) %4d '", name, argCount, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

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
int disassembleInstruction(Chunk* chunk, int offset)
{
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);
    case OP_PRINT:
      return simpleInstruction("OP_PRINT", offset);
    case OP_INHERIT:
      return simpleInstruction("OP_INHERIT", offset);
    case OP_GET_SUPER:
      return constantInstruction("OP_GET_SUPER", chunk, offset);
    case OP_SUPER_INVOKE:
      return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
    case OP_CLOSURE: {
      offset++;
      uint8_t constant = chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printValue(chunk->constants.values[constant]);
      printf("\n");

      ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf("%04d      |                     %s %d\n",
               offset - 2,
               isLocal ? "local" : "upvalue",
               index);
      }

      return offset;
    }
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
      return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byteInstruction("OP_CALL", chunk, offset);
    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OP_METHOD:
      return constantInstruction("OP_METHOD", chunk, offset);
    case OP_CLASS:
      return constantInstruction("OP_CLASS", chunk, offset);
    case OP_GET_PROPERTY:
      return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
      return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_INVOKE:
      return invokeInstruction("OP_INVOKE", chunk, offset);
      // TODO: fix debugging of this instructions
    case OP_BUILD_LIST:
      return simpleInstruction("OP_BUILD_LIST", offset);
    case OP_INDEX_GET:
      return simpleInstruction("OP_INDEX_GET", offset);
    case OP_INDEX_SET:
      return simpleInstruction("OP_INDEX_SET", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
