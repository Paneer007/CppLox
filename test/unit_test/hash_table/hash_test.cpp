#include <chrono>
#include <iostream>

#include <omp.h>
#include <string.h>

enum TestType
{
  HASH_8,
  HASH_32,
  HASH_128,
  HASH_512,
  HASH_1024
};

constexpr auto hash_8_str = "test";
constexpr auto hash_32_str = "j3K8d7F2p9Y5b4h1a6zXw0cQmE5Rabcd";
constexpr auto hash_128_str =
    "KwPqDHybFzU7lR6aN5iG4e3c2v1t0s9m8r7q6p5o4n3l2k1j0h9g8f7e6d5c4b3a2"
    "KwPqDHybFzU7lR6aN5iG4e3c2v1t0s9m8r7q6p5o4n3l2k1j0h9g8f7e6d5c4b3";
constexpr auto hash_512_str =
    "j3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfghjklzx"
    "cvbnmj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiop";

constexpr auto hash_1024_str =
    "j3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfghjklzx"
    "cvbnmj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "cvbnmj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "j3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfghjklzx"
    "cvbnmj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwertyuiopasdfgh"
    "jklzxj3K8d7F2p9Y5b4h1a6zXw0cQmE5R7tUioplkjhgfdsa1234567890qwer";

static inline uint32_t murmur_32_scramble(uint32_t k)
{
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}
uint32_t hashString(const char* key, size_t len)
{
  uint32_t seed = 0x00000000;
  uint32_t h = seed;
  uint32_t k;

  int num_threads = omp_get_max_threads();

#pragma omp parallel for reduction(^ : h)
  for (size_t i = 0; i < len >> 2; i++) {
    auto temp_k = key + i * sizeof(uint32_t);
    memcpy(&k, temp_k, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  k = 0;
  for (size_t i = len & 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
  h ^= murmur_32_scramble(k);
  /* Finalize. */
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

static

    void
    test_function(TestType func, const char* msg)
{
  std::cout << " ======== " << msg << " ======== " << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  uint32_t res = 0;
  switch (func) {
    case HASH_8:
      res = hashString(hash_8_str, 4);
      break;
    case HASH_32:
      res = hashString(hash_32_str, 32);
      break;
    case HASH_128:
      res = hashString(hash_128_str, 128);
      break;
    case HASH_512:
      res = hashString(hash_512_str, 512);
      break;
    case HASH_1024:
      res = hashString(hash_1024_str, 1024);
      break;
    default:
      break;
  }

  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "Output: " << res << std::endl;

  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << " nanoseconds"
            << std::endl;

  std::cout << "\n";
  ;
}

void test_hash()
{
  test_function(HASH_8, "HASH_8");
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