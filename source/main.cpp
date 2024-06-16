#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "vm.hpp"

class CppLox
{
private:
  /*
   *
   */
  static void repl()
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

  static char* readFile(const char* path)
  {
    auto file = fopen(path, "rb");
    if (file == NULL) {
      fprintf(stderr, "Could not open file \"%s\".\n", path);
      exit(74);
    }
    fseek(file, 0L, SEEK_END);
    auto fileSize = ftell(file);
    rewind(file);

    auto buffer = new char[fileSize + 1];
    if (buffer == NULL) {
      fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
      exit(74);
    }
    auto bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
      fprintf(stderr, "Could not read file \"%s\".\n", path);
      exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
  }

  static void runFile(const char* path)
  {
    auto vm = VM::getVM();
    auto source = readFile(path);
    InterpretResult result = vm->interpret(source);
    delete[] source;

    if (result == INTERPRET_COMPILE_ERROR)
      exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
      exit(70);
  }

public:
  /**
   * Main entry point of execution
   */
  int execute(int argc, const char* argv[])
  {
    Chunk chunk;
    auto vm = VM::getVM();
    vm->initVM();
    if (argc == 1) {
      repl();
    } else if (argc == 2) {
      runFile(argv[1]);
    } else {
      fprintf(stderr, "Usage: clox [path]\n");
      exit(64);
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