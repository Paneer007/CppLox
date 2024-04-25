#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "CppLox" ? 0 : 1;
}
