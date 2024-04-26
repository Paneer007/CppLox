#include "compiler.hpp"

#include <stdio.h>

#include "common.hpp"
#include "scanner.hpp"

int count = 0;
void compile(const char* source)
{
  auto scanner = Scanner::getScanner();
  scanner->initScanner(source);
  int line = -1;
  for (;;) {
    Token token = scanner->scanToken();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if (count == 10) {
      count = 0;
      break;
    }
    count += 1;

    if (token.type == TOKEN_EOF)
      break;
  }
}