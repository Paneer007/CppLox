#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "vm.hpp"

/**
 * @brief The CppLox Interpreter Class
 */
class CppLox
{
private:
  /**
   * @brief the Read-Eval-Print-Loop
   */
  void repl()
  {
    auto vm = VM::getVM();

    char line[1024];
    while (true) {
      printf("> ");

      if (!fgets(line, sizeof(line), stdin)) {
        printf("\n");
        break;
      }

      vm->interpret(line);
    }
  }

  /**
   * @brief Reads the contents of a file into a newly allocated buffer.
   *
   * @param path The path to the file to be read.
   * @return A pointer to the newly allocated buffer containing the file
   * contents, or NULL on failure.
   */
  char* readFile(const char* path)
  {
    // Opens file in read binary mode.
    // Checks if file pointer is null, exits with error if so.
    // Seeks to end of file, gets file size in bytes and returns to starting
    // position.
    auto file = fopen(path, "rb");
    if (file == NULL) {
      fprintf(stderr, "Could not open file \"%s\".\n", path);
      exit(74);
    }
    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Read the contents of the source file
    // Create a buffer of file size and read the source file
    // Run NULL pointer checks
    // Terminate with end-of-file character
    auto buffer = new char[fileSize + 1];
    if (buffer == NULL) {
      fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
      exit(74);
    }
    long bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
      fprintf(stderr, "Could not read file \"%s\".\n", path);
      exit(74);
    }
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
  }

  /**
   * @brief Run the file passed as parameter through CppLox VM
   *
   * @param path The source file
   */
  void runFile(const char* path)
  {
    auto vm = VM::getVM();
    auto source = this->readFile(path);
    InterpretResult result = vm->interpret(source);
    delete[] source;
    if (result == INTERPRET_COMPILE_ERROR)
      exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
      exit(70);
  }

public:
  /**
   * @brief Main entry point of execution
   *
   * @param argc Number of arguments
   * @param argv Array of arguments
   * @return int Status code of execution
   */
  int execute(int argc, const char* argv[])
  {
    Chunk chunk;
    auto vm = VM::getVM();
    vm->initVM();

    switch (argc) {
      case 1:
        repl();
        break;
      case 2:
        runFile(argv[1]);
        break;
      default:
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
        break;
    }

    vm->freeVM();
    return 0;
  }
};

int main(int argc, const char* argv[])
{
  CppLox program = CppLox();
  return program.execute(argc, argv);
}