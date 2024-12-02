#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <immintrin.h>
#include <smmintrin.h>
#include <stdlib.h>

const auto THRESHOLD_FACTOR = 0.75;

/* MEMORY MANAGEMENT FUNCTION */

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL)
    exit(1);
  return result;
}

template<typename T>
inline T* GROW_ARRAY(void* pointer, int oldCount, int newCount)
{
  return (T*)reallocate(
      pointer, sizeof(T) * (oldCount), sizeof(T) * (newCount));
}

template<typename T>
inline T* ALLOCATE(int count)
{
  return (T*)reallocate(NULL, 0, sizeof(T) * (count));
}

/* METADATA EXTRACTION OPERATIONS */

static inline std::uint32_t H1(std::uint32_t x)
{
  return ((x) >> 8);
}

static inline std::uint32_t H2(std::uint32_t x)
{
  return ((x) & ((1 << 8) - 1)) >> 1;
}

static inline bool TAG(std::uint32_t x)
{
  return (bool)(x & 1);
}

/* FNV-1a HASH FUNCTION */

static std::uint32_t hashString(std::string key)
{
  uint32_t hash = 6656037u;
  for (int i = 0; i < key.size(); i++) {
    hash ^= (uint8_t)key[i];
    hash *= 1099511628211;
  }
  std::cout << hash << std::endl;
  return hash;
}

/* UTILITY FUNCTIONS */

static inline bool thresholdCrossed(int count, int capacity)
{
  return count + 1 > capacity * THRESHOLD_FACTOR;
}

std::vector<int> getSetBitIndexes(int num)
{
  std::vector<int> indexes;
  int index = 7;

  while (num > 0) {
    if (num & 1) {
      // std::cout << "pushed back: " << index << std::endl;
      indexes.push_back(index);
    }
    num >>= 1;
    index--;
  }

  return indexes;
}

template<class T>
inline void Log(const __m256i& value)
{
  const size_t n = sizeof(__m256i) / sizeof(T);
  T buffer[n];
  _mm256_storeu_si256((__m256i*)buffer, value);
  for (int i = 0; i < n; i++)
    std::cout << buffer[i] << " ";
  std::cout << std::endl;
}
bool get_mode = false;

class Entry
{
public:
  std::string key;
  std::string val;
  Entry()
  {
    key = "";
    val = "";
  }
};

class MetaDataGroup
{
  __m256i elements;

public:
  MetaDataGroup() { this->elements = _mm256_setzero_si256(); }
  std::vector<int> checkElement(std::uint32_t h2)
  {
    // convert it to a mask value
    auto h2_mask = h2 << 1;  // Shifting by one bit to allow tag bit in mask
    h2_mask = h2_mask | 1;  // element must exist by setting tag bit to 1
    __m256i maskVec = _mm256_set1_epi32(
        h2_mask);  // Creating 4 copies of the h2 mask and using this as the 256
                   // bit vector instruction mask
    __m256i cmpResult = _mm256_cmpeq_epi32(
        elements, maskVec);  // comparing each mask and returning 1 if the masks
                             // match or not
    auto resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(
        cmpResult));  // Converting this mask from _m256d to integer
    resultMask = reverseBits(resultMask);  // For alignment  of indexes
    return getSetBitIndexes(resultMask);
  }

  std::vector<int> checkNulls()
  {
    // uint32_t mask = 0;
    // __m256i maskVec = _mm256_set1_epi32(mask);
    // __m256i cmpResult = _mm256_cmpeq_epi32(elements, maskVec);
    // auto resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(cmpResult));
    __m256i zeros = _mm256_setzero_si256();
    __m256i cmp = _mm256_cmpeq_epi32(elements, zeros);
    // Extract the mask of the comparison (each bit represents a 32-bit lane)
    int mask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
    // mask = reverseBits(mask);
    std::cout << "the null mask: " << mask << std::endl;
    return getSetBitIndexes(mask);
  }

  int reverseBits(int n)
  {
    n = ((n >> 1) & 0x55555555)
        | ((n & 0x55555555) << 1);  // Swap odd and even bits
    n = ((n >> 2) & 0x33333333)
        | ((n & 0x33333333) << 2);  // Swap consecutive pairs
    n = ((n >> 4) & 0x0F0F0F0F)
        | ((n & 0x0F0F0F0F) << 4);  // Swap nibbles (4 bits)
    n = ((n >> 8) & 0x00FF00FF)
        | ((n & 0x00FF00FF) << 8);  // Swap bytes (8 bits)
    n = (n >> 16) | (n << 16);  // Swap 16-bit chunks

    return n;
  }

  std::vector<int> checkTombstone()
  {
    __m256i rightmostBitMask = _mm256_set1_epi32(1);
    __m256i result = _mm256_and_si256(elements, rightmostBitMask);
    __m256i cmp = _mm256_cmpeq_epi32(result, rightmostBitMask);
    int resultMask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
    resultMask = reverseBits(resultMask);
    auto tempRes = getSetBitIndexes(resultMask);
    for (auto& x : tempRes) {
      std::cout << x << std::endl;
    }
    std::cout << "tombstone end" << std::endl;

    return getSetBitIndexes(resultMask);
  }

  void setBit(int index, int h2)
  {
    // std::cout << "The set bit: " << index << std::endl;
    // Log<long>(elements);

    int lane = index;  // 0 to 3 for each 32-bit segment
    uint32_t mask_value = 1 | (h2 << 1);
    __m256i mask;
    switch (lane) {
      case 0:
        mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, mask_value);
        break;
      case 1:
        mask = _mm256_set_epi32(0, 0, 0, 0, 0, 0, mask_value, 0);
        break;
      case 2:
        mask = _mm256_set_epi32(0, 0, 0, 0, 0, mask_value, 0, 0);
        break;
      case 3:
        mask = _mm256_set_epi32(0, 0, 0, 0, mask_value, 0, 0, 0);
        break;
      case 4:
        mask = _mm256_set_epi32(0, 0, 0, mask_value, 0, 0, 0, 0);
        break;
      case 5:
        mask = _mm256_set_epi32(0, 0, mask_value, 0, 0, 0, 0, 0);
        break;
      case 6:
        mask = _mm256_set_epi32(0, mask_value, 0, 0, 0, 0, 0, 0);
        break;
      case 7:
        mask = _mm256_set_epi32(mask_value, 0, 0, 0, 0, 0, 0, 0);
        break;
      default:
        mask = _mm256_setzero_si256();  // Invalid i

        // std::cout << elements << std::endl
    }
    elements = _mm256_or_si256(elements, mask);
    // Log<long>(elements);
  };
};

class Group
{
  MetaDataGroup mdg;

public:
  std::vector<Entry> entries;

  Group() { this->entries = std::vector<Entry>(4); }

  inline std::vector<int> search_element(int h2, std::string key)
  {
    return mdg.checkElement(h2);
  }

  inline std::vector<int> search_null() { return mdg.checkNulls(); }

  inline std::vector<int> search_tombstone() { return mdg.checkTombstone(); }

  inline void set_element(int index, int h2, std::string key, std::string value)
  {
    entries[index].key = key;
    entries[index].val = value;
    mdg.setBit(index, h2);
  }
};

class HashMap
{
  std::vector<Group> groups;

  int count;

public:
  HashMap() { this->groups = std::vector<Group>(8); }

  ~HashMap() {}

  bool get(std::string key, std::string& value)
  {
    std::cout << "getting element" << std::endl;
    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);
    auto index = h1 % groups.size();
    while (true) {
      std::cout << "next index" << std::endl;
      auto matches = groups[index].search_element(h2, key);
      if (matches.size()) {
        for (auto x : matches) {
          auto element = groups[index].entries[x];
          if (element.key.size() == key.size() && element.key == key) {
            std::cout << "look here" << std::endl;
            value = element.val;
            return true;
          }
        }
      }

      // check for null elements, if present return false
      auto null = groups[index].search_null();
      if (null.size()) {
        return false;
      }

      // start next iterationm
      index++;
    }
  }

  void set(std::string key, std::string value)
  {
    std::cout << "setting element" << std::endl;

    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);
    auto index = h1 % groups.size();
    std::pair<int, int> tombstone_index = {-1, -1};
    while (true) {
      std::cout << "next index" << index << std::endl;
      auto matches = groups[index].search_element(h2, key);
      if (matches.size()) {
        for (auto x : matches) {
          auto element = groups[index].entries[x];
          if (element.key.size() == key.size() && element.key == key) {
            std::cout << "Look here 1" << std::endl;
            groups[index].entries[x].val == value;  // performing update
            return;
          }
        }
      }
      if (tombstone_index.first == -1) {
        auto tombstones = groups[index].search_tombstone();
        if (tombstones.size()) {
          tombstone_index.first = index;
          tombstone_index.second = tombstones[0];
        }
      }
      auto null = groups[index].search_null();
      if (null.size()) {
        if (tombstone_index.first != -1) {
          std::cout << "Look here 2" << std::endl;
          groups[tombstone_index.first].set_element(
              tombstone_index.second, h2, key, value);
          return;
        }
        std::cout << "Look here 3" << std::endl;
        auto null_index = null[0];
        groups[index].set_element(null_index, h2, key, value);
      }
      index = (index + 1) % groups.size();
      exit(0);
    }
  }

  bool del(std::string key)
  {
    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);
    auto index = h1 % groups.size();
    while (true) {
      auto matches = groups[index].search_element(h2, key);
      if (matches.size()) {
        for (auto x : matches) {
          auto element = groups[index].entries[x];
          if (element.key.size() == key.size() && element.key == key) {
            groups[index].set_element(x, 0, "", "");
            return true;
          }
        }
      }

      // check for null elements, if present return false
      auto null = groups[index].search_null();
      if (null.size()) {
        return false;
      }

      // start next iteration
      index = (index + 1) % groups.size();
    }
  }
};

int main()
{
  std::cout << "here" << std::endl;

  auto mp = HashMap();
  std::vector<std::string> arr = {
      "Hello", "World", "Today", "I", "have", "tried", "SSE", "HASH"};
  for (auto& x : arr) {
    mp.set(x, x + "_value");
  }
  std::string value;
  auto res = mp.get("World", value);
  std::cout << value << res << std::endl;

  mp.del("World");
  res = mp.get("World", value);
  std::cout << value << res << std::endl;
  return 0;
}