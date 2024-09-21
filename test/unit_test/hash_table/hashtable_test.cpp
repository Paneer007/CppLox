#include <chrono>
#include <iostream>
#include <vector>

#include <bits/stdc++.h>
#include <omp.h>

#include "../../../source/chunk.cpp"
#include "../../../source/chunk.hpp"
#include "../../../source/compiler.cpp"
#include "../../../source/compiler.hpp"
#include "../../../source/debug.cpp"
#include "../../../source/debug.hpp"
#include "../../../source/dispatcher.cpp"
#include "../../../source/dispatcher.hpp"
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
  HASH_16384,
  HASH_262144,
  HASH_4194304,
  HASH_33554432,
  HASH_1000000000
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
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < len; i++) {
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
    // printf("%d \n", ((ObjString*)obj_key)->hash);
    auto temp = obj_key;
    table.tableSet(temp, OBJ_VAL(temp));
  }
  // printf("Search time \n");
  // Execute search logic
  for (int i = 0; i < len / 4; i++) {
    // std::cout << i << std::endl;
    auto obj_key = obj_keys[rand() % keys.size()];
    // printf("%d \n", ((ObjString*)obj_key)->hash);
#ifndef ENABLE_MP
    table.tableFindString(((ObjString*)obj_key)->chars,
                          ((ObjString*)obj_key)->length,
                          ((ObjString*)obj_key)->hash);
#else
    auto x = table.tableFindString(((ObjString*)obj_key)->chars,
                                   ((ObjString*)obj_key)->length,
                                   ((ObjString*)obj_key)->hash,
                                   ((ObjString*)obj_key)->hash2);
    if (x == NULL) {
      printf("Error \n");
      exit(0);
    }
    printf("%s \n", x->chars);
#endif
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  table.freeTable();
  auto x = duration.count();
  return x;
}

typedef std::string str;

int64_t test_vector(int len)
{
  // Table table;
  // table.initTable();
  // std::vector<std::string> keys;
  // std::vector<Obj*> obj_keys;
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::pair<str, str>> arrays;
  std::vector<std::string> keys;
  for (int i = 0; i < len; i++) {
    auto key = gen_random(KEY_SIZE);
    keys.push_back(key);
    arrays.push_back({key, key});
  }

  // Execute search logic
  for (int i = 0; i < len / 4; i++) {
    auto obj_key = arrays[rand() % keys.size()];
    auto x = find(arrays.begin(), arrays.end(), obj_key);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  // table.freeTable();
  auto x = duration.count();
  return x;
}

int64_t test_map(int len)
{
  auto start = std::chrono::high_resolution_clock::now();
  std::unordered_map<std::string, std::string> mp;
  std::vector<std::string> keys;
  for (int i = 0; i < len; i++) {
    auto key = gen_random(KEY_SIZE);
    keys.push_back(key);
    mp[key] = key;
  }

  // Execute search logic
  for (int i = 0; i < len / 4; i++) {
    auto obj_key = keys[rand() % keys.size()];
    auto x = mp[obj_key];
    // auto value = OBJ_VAL(obj_key);
    // table.tableGet((ObjString*)obj_key, &value);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

  auto x = duration.count();
  return x;
}

static void test_custom_function(TestType func, const char* msg)
{
  std::cout << " ======== " << msg << " ======== " << std::endl;
  // Warm up

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
      break;
    case HASH_16384:
      x = test_table(16384 * 1.5);
      break;
    case HASH_262144:
      x = test_table(262144 * 1.5);
      break;
    case HASH_4194304:
      x = test_table(4194304 * 1.5);
      break;
    case HASH_33554432:
      x = test_table(33554432 * 1.5);
      break;
    default:
      break;
  }

  std::cout << "Execution time: " << x << " nanoseconds" << std::endl;

  std::cout << "\n";
}

static void test_vector_function(TestType func, const char* msg)
{
  std::cout << " ======== " << msg << " ======== " << std::endl;
  // Warm up
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();

  int64_t x;
  switch (func) {
    case HASH_8:
      x = test_vector(8 * 1.5);
      break;
    case HASH_16:
      x = test_vector(16 * 1.5);
      break;
    case HASH_32:
      x = test_vector(32 * 1.5);
      break;
    case HASH_128:
      x = test_vector(128 * 1.5);
      break;
    case HASH_512:
      x = test_vector(512 * 1.5);
      break;
    case HASH_1024:
      x = test_vector(1024 * 1.5);
      break;
    case HASH_16384:
      x = test_vector(16384 * 1.5);
      break;
    case HASH_262144:
      x = test_vector(262144 * 1.5);
      break;
    case HASH_4194304:
      x = test_vector(4194304 * 1.5);
      break;
    case HASH_33554432:
      x = test_vector(33554432 * 1.5);
      break;
    default:
      break;
  }

  std::cout << "Execution time: " << x << " nanoseconds" << std::endl;

  std::cout << "\n";
}

static void test_map_function(TestType func, const char* msg)
{
  std::cout << " ======== " << msg << " ======== " << std::endl;
  // Warm up
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();

  int64_t x;
  switch (func) {
    case HASH_8:
      x = test_map(8 * 1.5);
      break;
    case HASH_16:
      x = test_map(16 * 1.5);
      break;
    case HASH_32:
      x = test_map(32 * 1.5);
      break;
    case HASH_128:
      x = test_map(128 * 1.5);
      break;
    case HASH_512:
      x = test_map(512 * 1.5);
      break;
    case HASH_1024:
      x = test_map(1024 * 1.5);
      break;
    case HASH_16384:
      x = test_map(16384 * 1.5);
      break;
    case HASH_262144:
      x = test_map(262144 * 1.5);
      break;
    case HASH_4194304:
      x = test_map(4194304 * 1.5);
      break;
    case HASH_33554432:
      x = test_map(33554432 * 1.5);
      break;
    default:
      break;
  }

  std::cout << "Execution time: " << x << " nanoseconds" << std::endl;

  std::cout << "\n";
}

void test_hash()
{
  // test_custom_function(HASH_8, "HASH_8");
  // test_custom_function(HASH_16, "HASH_16");
  // test_custom_function(HASH_32, "HASH_32");
  // test_custom_function(HASH_128, "HASH_128");
  // test_custom_function(HASH_512, "HASH_512");
  // test_custom_function(HASH_1024, "HASH_1024");
  // test_custom_function(HASH_16384, "HASH_16384");
  // test_custom_function(HASH_262144, "HASH_262144");
  test_custom_function(HASH_4194304, "HASH_4194304");
  // test_custom_function(HASH_33554432, "HASH_33554432");
  // test_custom_function(HASH_1000000000, "HASH_1000000000");
}

void test_vector()
{
  test_vector_function(HASH_8, "HASH_8");
  test_vector_function(HASH_16, "HASH_16");
  test_vector_function(HASH_32, "HASH_32");
  test_vector_function(HASH_128, "HASH_128");
  test_vector_function(HASH_512, "HASH_512");
  test_vector_function(HASH_1024, "HASH_1024");
  test_vector_function(HASH_16384, "HASH_16384");
  test_vector_function(HASH_262144, "HASH_262144");
  test_vector_function(HASH_4194304, "HASH_4194304");
  test_vector_function(HASH_33554432, "HASH_33554432");
}

void test_mapr()
{
  test_map_function(HASH_8, "HASH_8");
  test_map_function(HASH_16, "HASH_16");
  test_map_function(HASH_32, "HASH_32");
  test_map_function(HASH_128, "HASH_128");
  test_map_function(HASH_512, "HASH_512");
  test_map_function(HASH_1024, "HASH_1024");
  test_map_function(HASH_16384, "HASH_16384");
  test_map_function(HASH_262144, "HASH_262144");
  test_map_function(HASH_4194304, "HASH_4194304");
  test_map_function(HASH_33554432, "HASH_33554432");
}

void test_map() {}

int main()
{
  auto x = Dispatcher::getDispatcher();
  x->dispatchThread(NULL);
  std::cout << "====== HASH TEST ======" << std::endl;
  test_hash();
  // std::cout << "====== VECTOR TEST ======" << std::endl;
  // test_vector();
  // std::cout << "====== SET TEST ======" << std::endl;
  // test_mapr();
  return 0;
}