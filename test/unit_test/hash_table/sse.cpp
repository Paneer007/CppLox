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

static inline std::uint64_t H1(std::uint64_t x)
{
  return ((x) >> 8);
}

static inline std::uint64_t H2(std::uint64_t x)
{
  return ((x) & ((1 << 8) - 1)) >> 1;
}

static inline bool TAG(std::uint64_t x)
{
  return (bool)(x & 1);
}

/* FNV-1a HASH FUNCTION */

static std::uint64_t hashString(std::string key)
{
  uint64_t hash = 14695981039346656037u;
  for (int i = 0; i < key.size(); i++) {
    hash ^= (uint8_t)key[i];
    hash *= 1099511628211;
  }
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
  int index = 3;

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

/* LOGGING TOOLS */
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
};

class MetaDataGroup
{
  __m256i elements;

public:
  std::vector<int> checkElement(std::uint64_t h2)
  {
    // convert it to a mask value

    auto h2_mask = h2 << 1;  // Shifting by one bit to allow tag bit in mask
    h2_mask = h2_mask | 1;  // element must exist by setting tag bit to 1
    __m256i maskVec = _mm256_set1_epi64x(
        h2_mask);  // Creating 4 copies of the h2 mask and using this as the 256
                   // bit vector instruction mask
    __m256i cmpResult = _mm256_cmpeq_epi64(
        elements, maskVec);  // comparing each mask and returning 1 if the masks
                             // match or not
    auto resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(
        cmpResult));  // Converting this mask from _m256d to integer
    resultMask = reverseBits(resultMask);  // For alignment  of indexes

    if (get_mode) {
      std::cout << "elements: ";
      Log<long>(elements);
      std::cout << "mask Vector: ";
      Log<long>(maskVec);
      std::cout << "cmp result: ";
      Log<long>(cmpResult);
    }
    return getSetBitIndexes(resultMask);
  }

  std::vector<int> checkAllElement(std::uint64_t h2)
  {
    // convert it to a mask value
    auto h2_mask = h2 << 1;  // Shifting by one bit to allow tag bit in mask
    h2_mask = h2_mask | 1;  // element must exist by setting tag bit to 1
    __m256i maskVec = _mm256_set1_epi64x(
        h2_mask);  // Creating 4 copies of the h2 mask and using this as the 256
                   // bit vector instruction mask
    __m256i cmpResult = _mm256_cmpeq_epi64(
        elements, maskVec);  // comparing each mask and returning 1 if the masks
                             // match or not
    auto resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(
        cmpResult));  // Converting this mask from _m256d to integer
    resultMask = reverseBits(resultMask);  // For alignment  of indexes

    if (get_mode) {
      std::cout << "elements: ";
      Log<long>(elements);
      std::cout << "mask Vector: ";
      Log<long>(maskVec);
      std::cout << "cmp result: ";
      Log<long>(cmpResult);
    }
    return getSetBitIndexes(resultMask);
  }

  int reverseBits(int n)
  {
    return ((n & 0x1) << 3) | ((n & 0x2) << 1) | ((n & 0x4) >> 1)
        | ((n & 0x8) >> 3);
  }

  std::vector<int> checkFreeElements()
  {
    __m256i rightmostBitMask = _mm256_set1_epi64x(1);
    __m256i result = _mm256_and_si256(elements, rightmostBitMask);
    __m256i cmp = _mm256_cmpeq_epi64(result, _mm256_setzero_si256());
    int resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
    resultMask = reverseBits(resultMask);
    return getSetBitIndexes(resultMask);
  }

  void setBit(int index, int h2)
  {
    // std::cout << "The set bit: " << index << std::endl;
    // Log<long>(elements);

    int lane = index;  // 0 to 3 for each 64-bit segment
    uint64_t mask_value = 1 | (h2 << 1);
    __m256i mask;
    switch (lane) {
      case 0:
        mask = _mm256_set_epi64x(0, 0, 0, mask_value);
        break;
      case 1:
        mask = _mm256_set_epi64x(0, 0, mask_value, 0);
        break;
      case 2:
        mask = _mm256_set_epi64x(0, mask_value, 0, 0);
        break;
      case 3:
        mask = _mm256_set_epi64x(mask_value, 0, 0, 0);
        break;
      default:
        mask = _mm256_setzero_si256();  // Invalid i

        // std::cout << elements << std::endl
    }
    elements = _mm256_or_si256(elements, mask);
    // Log<long>(elements);
  };
};

// The ratio of elements in mdg to entries is 1:4 i.e each group has metadata
// for 4 elements

class SSEHashMap
{
  MetaDataGroup* metadatagroups;
  int mdg_count;
  int mdg_capacity;

  void growArray() { std::cout << "WIP" << std::endl; }

  std::vector<std::pair<int, int>> findFilledEntryIndexes(int h1, int h2)
  {
    // searching MetaDataGroup
    auto startingIndex = h1 % mdg_capacity;
    if (startingIndex < 0) {
      startingIndex += mdg_capacity;
    }
    auto iter = startingIndex;
    do {
      auto elem = metadatagroups[iter];
      auto res = elem.checkElement(h2);
      if (res.size() != 0) {
        std::vector<std::pair<int, int>> temp;
        for (auto& x : res) {
          temp.push_back({iter, x});
        }
        return temp;
      }
      iter = (iter + 1) % mdg_capacity;
      // std::cout << "The elements are " << iter << std::endl;
    } while (iter != startingIndex);
    return {};
  }

  std::pair<int, int> findFreeEntryIndex(int h1, int h2)
  {
    if (thresholdCrossed(this->entries_count, this->entries_capacity)) {
      // std::cout << "Crossing threshold" << std::endl;
      this->growArray();
    }
    // searching MetaDataGroup
    auto startingIndex = h1 % mdg_capacity;
    if (startingIndex < 0) {
      startingIndex += mdg_capacity;
    }
    auto iter = startingIndex;
    do {
      auto elem = metadatagroups[iter];
      auto res = elem.checkFreeElements();
      if (res.size() != 0) {
        std::vector<std::pair<int, int>> temp;
        auto x = res.back();

        metadatagroups[iter].setBit(x, h2);
        temp.push_back({iter, x});
        return temp[0];
      }
      iter = (iter + 1) % this->mdg_capacity;
    } while (iter != startingIndex);
    throw std::runtime_error("No free element available");
  }

public:
  Entry* entries;
  int entries_count;
  int entries_capacity;
  SSEHashMap()
  {
    this->entries_count = 0;
    this->entries_capacity = 8;
    this->entries = new Entry[8];

    this->mdg_capacity = 2;
    this->mdg_count = 0;
    this->metadatagroups = new MetaDataGroup[2];
  }

  ~SSEHashMap()
  {
    this->entries_count = 0;
    this->entries_capacity = 0;
    // delete entries;

    this->mdg_capacity = 0;
    this->mdg_count = 0;
    delete metadatagroups;
  }

  // void set(std::string key, std::string value)
  // {
  //   auto hash = hashString(key);
  //   auto h1 = H1(hash);
  //   auto h2 = H2(hash);
  //   auto entries = this->findFilledEntryIndexes(h1, h2);
  //   if (!entries.size()) {
  //     auto free_entries = this->findFreeEntryIndex(h1, h2);
  //     auto index = free_entries.first * 4 + free_entries.second;
  //     this->entries[index].key = key;
  //     this->entries[index].val = value;
  //   } else {
  //     for (auto& x : entries) {
  //       auto index = x.first * 4 + x.second;
  //       if (this->entries[index].key == key) {
  //         this->entries[index].val = value;
  //         break;
  //       }
  //     }
  //   }
  // }

  void set(std::string key, std::string value)
  {
    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);

    auto starting_index = h1 % mdg_capacity;
    starting_index += mdg_capacity;
    starting_index %= mdg_capacity;
    auto iter = starting_index;

    do {
      auto elem = metadatagroups[iter];
      auto res = elem.checkElement(h2);
      if (res.size() > 0) {
        for (auto& x : res) {
          auto index = iter * 4 + x;
          if (this->entries[index].key == key) {
            this->entries[index].val = value;
          }
        }
      }
    } while (iter != starting_index);
  }

  std::string get(std::string key)
  {
    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);
    auto filled_entries = this->findFilledEntryIndexes(h1, h2);
    if (!filled_entries.size()) {
      throw std::runtime_error("Element does not exist");
    }
    for (auto& x : filled_entries) {
      auto index = x.first * 4 + x.second;
      if (this->entries[index].key == key) {
        return this->entries[index].val;
      }
    }
    throw std::runtime_error("Element does not exist here");
  }
};

int main()
{
  auto mp = SSEHashMap();
  std::vector<std::string> arr = {
      "Hello", "World", "Today", "I", "have", "tried", "SSE", "HASH"};
  for (auto& x : arr) {
    mp.set(x, x + "SSE");
  }
  get_mode = true;
  auto res = mp.get("Hello");
  std::cout << res << std::endl;
  return 0;
}

/**
 * Possible errors, when set finds element but it doesn't exactly stop there,
 * i.e need to test all elements then continue. Need to redo it with continuable
 * indicies. Something like stream indices in a weird way How to do it, think
 * about it
 */