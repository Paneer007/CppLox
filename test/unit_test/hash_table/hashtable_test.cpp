#include <chrono>
#include <iostream>
#include <vector>

#include <omp.h>
#include <string.h>

#include "../../../source/chunk.cpp"
#include "../../../source/chunk.hpp"
#include "../../../source/compiler.cpp"
#include "../../../source/compiler.hpp"
#include "../../../source/debug.cpp"
#include "../../../source/debug.hpp"
#include "../../../source/memory.cpp"
#include "../../../source/memory.hpp"
#include "../../../source/object.cpp"
#include "../../../source/object.hpp"
#include "../../../source/scanner.cpp"
#include "../../../source/scanner.hpp"
#include "../../../source/table.cpp"
#include "../../../source/table.hpp"
#include "../../../source/value.cpp"
#include "../../../source/value.hpp"
#include "../../../source/vm.cpp"
#include "../../../source/vm.hpp"

enum TestType
{
  HASH_8,
  HASH_16,
  HASH_32,
  HASH_128,
  HASH_512,
  HASH_1024,
  HASH_10241024
};

const int KEY_SIZE = 10;

std::string gen_random(const int len)
{
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  return tmp_s;
}

static ObjString* temp_allocateString(char* chars,
                                      int length,
                                      uint32_t hash,
                                      uint32_t hash2)
{
  auto string = ALLOCATE_OBJ<ObjString>(OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
#ifdef ENABLE_MP
  string->hash2 = hash2;
#endif
  return string;
}

int64_t test_table(int len)
{
  Table table;
  table.initTable();
  std::vector<std::string> keys;
  std::vector<Obj*> obj_keys;

  for (int i = 0; i < len; i++) {
    printf("INSERTING ELEMENT %d \n", i);
    auto key = gen_random(KEY_SIZE);
    keys.push_back(key);
    char* chars = &key[0];
    int length = key.size();
    uint32_t hash = hashString(chars, KEY_SIZE);
#ifdef ENABLE_MP
    uint32_t hash2 = hash2ndString(chars, KEY_SIZE);
#else
    uint32_t hash2 = 0;
#endif
    auto heapChars = ALLOCATE<char>(length + 1);
    memcpy(heapChars, chars, length);

    heapChars[length] = '\0';

    auto obj_key = temp_allocateString(heapChars, length, hash, hash2);
    obj_keys.push_back(obj_key);

    auto temp = obj_key;
    table.tableSet(temp, OBJ_VAL(temp));
  }

  printf("BEGIN SELECTING  VALUES \n");

  auto start = std::chrono::high_resolution_clock::now();
  // Execute search logic
  for (int i = 0; i < len / 4; i++) {
    auto obj_key = obj_keys[rand() % keys.size()];
    auto value = OBJ_VAL(obj_key);
    table.tableGet((ObjString*)obj_key, &value);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  table.freeTable();
  auto x = duration.count();
  return x;
}

static void test_function(TestType func, const char* msg)
{
  std::cout << " ======== " << msg << " ======== " << std::endl;
  // Warm up
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();

  int64_t x;
  switch (func) {
    case HASH_8:
      x = test_table(8 * 1.5);
      break;
    case HASH_16:
      x = test_table(16 * 1.5);
      break;
    case HASH_32:
      x = test_table(32 * 1.5);
      break;
    case HASH_128:
      x = test_table(128 * 1.5);
      break;
    case HASH_512:
      x = test_table(512 * 1.5);
      break;
    case HASH_1024:
      x = test_table(1024 * 1.5);
    case HASH_10241024:
      x = test_table(1024 * 1.5);
      break;
    default:
      break;
  }

  std::cout << "Execution time: " << x << " nanoseconds" << std::endl;

  std::cout << "\n";
}

void test_hash()
{
  test_function(HASH_8, "HASH_8");
  test_function(HASH_16, "HASH_16");
  test_function(HASH_32, "HASH_32");
  test_function(HASH_128, "HASH_128");
  test_function(HASH_512, "HASH_512");
  test_function(HASH_1024, "HASH_1024");
}

int main()
{
  test_hash();
  return 0;
}