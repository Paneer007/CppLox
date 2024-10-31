#include <iostream>
#include <string>
#include <vector>

#include <immintrin.h>
#include <smmintrin.h>

static inline uint64_t H2(uint64_t x)
{
  return ((x) & ((1 << 8) - 1)) >> 1;
}

static inline uint64_t H1(uint64_t x)
{
  return ((x) >> 8);
}

static inline uint64_t TAG(uint64_t x)
{
  return (x & 1);
}

static uint64_t hashString(std::string key)
{
  uint64_t hash = 14695981039346656037u;
  for (int i = 0; i < key.size(); i++) {
    hash ^= (uint8_t)key[i];
    hash *= 1099511628211;
  }
  return hash;
}

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
  int checkElement(uint64_t h2)
  {
    // convert it to a mask value
    auto h2_mask = h2 << 1;
    auto mask = h2_mask | 1;
    __m256i maskVec = _mm256_set1_epi64x(h2_mask);
    __m256i cmpResult = _mm256_cmpeq_epi64(elements, maskVec);
    auto resultMask = _mm256_movemask_pd(_mm256_castsi256_pd(cmpResult));
    std::cout << resultMask << std::endl;
    return resultMask;
  }
};

class MetaData
{
  int count;
  int capacity;
  std::vector<MetaDataGroup> meta_data_groups;

  void growArray() {}

public:
  MetaData()
  {
    this->count = 0;
    this->capacity = 0;
  }

  ~MetaData()
  {
    this->count = 0;
    this->capacity = 0;
  }

  int getGroupCount() { return count; }
  std::vector<std::pair<int, int>> findElementIndex(int h1, int h2)
  {
    // No elements in list -> return empty co-ordinates
    if (this->count == 0) {
      return {};
    }

    // get initial starting hash
    auto meta_data_groups_len = meta_data_groups.size();
    auto starting_index = h1 % meta_data_groups_len;

    auto iter = starting_index;

    do {
      auto elem = meta_data_groups[iter];
      auto res = elem.checkElement(h2);
      if (res != 0) {
        break;
      }
      iter++;
    } while (iter != starting_index);
  }

  std::pair<int, int> findFreeSlotIndex() {}

  std::pair<int, int> insertElement() {}
};

class SSEHashMap
{
  std::vector<Entry> entries;
  MetaData metadata;
  int count;

  void growMap() {}

  void findEntry(std::string key) {}

public:
  SSEHashMap() { this->count = 0; }

  ~SSEHashMap() { this->count = 0; }

  void set(std::string key, std::string value)
  {
    auto hash = hashString(key);
    auto h1 = H1(hash);
    auto h2 = H2(hash);
    auto elements = metadata.findElementIndex(h1, h2);
    if (!elements.size()){
      
    }else{

    }
  }

  std::string get(std::string key) {}
};

int main()
{
  std::cout << "LOOK HERE" << std::endl;
  auto mp = SSEHashMap();
  std::vector<std::string> arr = {
      "Hello", "World", "Today", "I", "have", "tried", "SSE", "INSTRUCTIONS"};
  for (auto& x : arr) {
    mp.set(x, x);
  }
  return 0;
}