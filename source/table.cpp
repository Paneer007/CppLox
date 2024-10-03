#include "table.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"

#ifdef ENABLE_MP
#  include <omp.h>
#endif

#ifdef ENABLE_MTHM
#  include <omp.h>
#endif

/**
 * @brief Maximum load factor for the hash table before resizing.
 *
 * This constant determines the upper limit of the table's load factor, which is
 * the ratio of the number of entries to the table's capacity. When the load
 * factor exceeds this value, the table is resized to improve performance.
 *
 * TODO: Tune this metric
 */
constexpr double TABLE_MAX_LOAD = 0.75;
/**
 * @brief Initializes an empty table.
 *
 * Resets the table's internal state, setting the count and capacity to zero and
 * the entries array to null.
 */
void Table::initTable()
{
  for (int i = 0; i < THREAD_COUNT; i++) {
    this->count[i] = 0;
    this->capacity[i] = 0;
  }
  this->entries = new Entry*[THREAD_COUNT];
  this->worklist =
      std::vector<std::vector<std::pair<ObjString*, Value>>>(THREAD_COUNT);
}

/**
 * @brief Finds an entry in a hash table.
 *
 * Searches for an entry with the given key within the specified hash table.
 * Handles collisions using linear probing. Returns a pointer to the found
 * entry, or a suitable empty or tombstone entry if not found.
 *
 * @param entries The array of hash table entries.
 * @param capacity The capacity of the hash table.
 * @param key The key to search for.
 * @return A pointer to the found entry, or an empty or tombstone entry if not
 * found.
 */

static Entry* findEntry(Entry* entries,
                        int capacity,
                        ObjString* key,
                        int thread_id = -1)
{
  Entry* tombstone = NULL;
#ifdef ENABLE_MTHM
  uint32_t index = key->hash % (capacity);

  for (;;) {
    Entry* entry = &entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }
    index = (index + key->hash2) % (capacity);
    // printf("%d %d \n", index, key->hash2);
  }

#else
  uint32_t index = key->hash % (capacity);

  for (;;) {
    Entry* entry = &entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }
    index = (index + 1) % (capacity);
  }

#endif
}

/**
 * @brief Resizes the hash table to the specified capacity.
 *
 * Creates a new hash table with the given capacity, rehashes existing
 * entries, and replaces the old table with the new one. Handles collisions
 * and tombstones during the rehashing process.
 *
 * @param capacity The new capacity for the hash table.
 */
void Table::adjustCapacity(int capacity, int index)
{
  // printf("new adjusting capacity %d \n", capacity);
  Entry* new_entries = ALLOCATE<Entry>(capacity);
  for (int i = 0; i < capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = NIL_VAL;
  }
  this->count[index] = 0;
  for (int i = 0; i < this->capacity[index]; i++) {
    Entry* entry = &this->entries[index][i];
    if (entry->key == NULL)
      continue;

    Entry* dest = findEntry(new_entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    this->count[index]++;
  }
  // printf("old capacity: %d \n", this->capacity[index]);
  FREE_ARRAY<Entry>(this->entries[index], this->capacity[index]);
  this->entries[index] = new_entries;
  this->capacity[index] = capacity;
}

/**
 * @brief Frees the memory allocated for the hash table.
 *
 * Deallocates the memory used by the hash table's entries array and resets
 * the table's state to an empty table.
 */
void Table::freeTable()
{
#ifdef ENABLE_MTHM

#endif

  for (int i = 0; i < THREAD_COUNT; i++) {
    delete this->entries[i];
  }
  worklist.clear();
  this->initTable();
}

// #ifdef ENABLE_MP
// void Table::applyWorklist()
// {
//   int n_items = this->EntriesWorkList.getLength();
//   if (n_items == 0) {
//     return;
//   }
//   // Parallel insert stuff
//   omp_lock_t* lock = (omp_lock_t*)malloc(capacity * sizeof(omp_lock_t));
//   for (int i = 0; i < capacity; i++) {
//     omp_init_lock(&(lock[i]));
//   }
// #  pragma omp parallel for num_threads(4)
//   for (int i = 0; i < n_items; i++) {
//     auto element = this->EntriesWorkList.getElement(i);
//     auto key = element->key;
//     auto value = element->value;

//     int counter = 0;
//     bool searchDone = false;
//     auto hash1 = key->hash & (capacity - 1);
//     auto hash2 = key->hash2 & (capacity - 1);
//     while (not searchDone and counter <= capacity) {
//       auto hashedValue = (hash1 + hash2 * counter) & (capacity - 1);
//       counter += 1;
//       if (this->entries[hashedValue].key == NULL) {
//         omp_set_lock(&(lock[hashedValue]));
//         if (this->entries[hashedValue].key == NULL) {
//           searchDone = true;
//           this->entries[hashedValue].key = key;
//           this->entries[hashedValue].value = value;
//           this->count++;
//         }
//         omp_unset_lock(&(lock[hashedValue]));
//       }
//     }
//   }

//   for (int i = 0; i < capacity; i++) {
//     omp_unset_lock(&(lock[i]));
//     omp_destroy_lock(&(lock[i]));
//   }

//   free(lock);

//   this->EntriesWorkList.clearWorkList();
// }

// #endif

void Table::clearWorklist()
{
  for (int i = 0; i < THREAD_COUNT; i++) {
    this->worklist[i].clear();
  }
}

#ifdef ENABLE_MTHM
void Table::applyWorklist()
{
#  pragma omp parallel for num_threads(THREAD_COUNT)
  for (int i = 0; i < THREAD_COUNT; i++) {
    auto curr_worklist = this->worklist[i];
    auto curr_entries = this->entries[i];

    for (int j = 0; j < curr_worklist.size(); j++) {
      auto element = &curr_worklist[j];
      Entry* entry =
          findEntry(curr_entries, this->capacity[i], element->first, i);
      bool isNewKey = entry->key == NULL;
      entry->key = element->first;
      entry->value = element->second;
    }
  }

  this->clearWorklist();
}

#endif

/**
 * @brief Sets a value in the hash table.
 *
 * Inserts or updates a key-value pair in the hash table. Handles resizing the
 * table if necessary to maintain the load factor. Returns `true` if a new key
 * was inserted, `false` if an existing key was updated.
 *
 * @param key The key to associate with the value.
 * @param value The value to store.
 * @return `true` if a new key was inserted, `false` otherwise.
 */
bool Table::tableSet(ObjString* key, Value value)
{
  // #ifdef ENABLE_MP
  //   if (this->count + 1 > this->capacity * TABLE_MAX_LOAD) {
  //     int capacity = GROW_CAPACITY(this->capacity);
  //     adjustCapacity(capacity);
  //     applyWorklist();
  //   }
  //   this->EntriesWorkList.writeWorkList(key, value);
  //   this->count += 1;

  // #else

#ifdef ENABLE_MTHM
  auto index = key->hash % THREAD_COUNT;
  if (this->count[index] + 1 > this->capacity[index] * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(this->capacity[index]);
    adjustCapacity(capacity, index);
    applyWorklist();
  }

  this->worklist[index].push_back({key, value});
  this->count[index] += 1;

#else
  if (this->count + 1 > this->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(this->capacity);
    adjustCapacity(capacity);
    this->capacity = capacity;
  }
  Entry* entry = findEntry(this->entries, this->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value))
    this->count++;
  if (isNewKey)
    this->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
#endif
  // #endif
}

/**
 * @brief Gets a value from the hash table.
 *
 * Retrieves the value associated with the given key from the hash table.
 * If the key is found, the corresponding value is stored in the provided
 * `value` pointer.
 *
 * @param key The key to search for.
 * @param value A pointer to store the found value.
 * @return `true` if the key is found and the value is stored, `false`
 * otherwise.
 */
bool Table::tableGet(ObjString* key, Value* value)
{
  // #ifdef ENABLE_MP
  //   if (this->EntriesWorkList.getLength() > 0) {
  //     applyWorklist();
  //   }
  // #endif

#ifdef ENABLE_MTHM
  auto hash_index = key->hash % (THREAD_COUNT);
  if (this->worklist[hash_index].size() > 0) {
    applyWorklist();
  }
#endif

  if (this->count[hash_index] == 0)
    return false;

#ifdef ENABLE_MTHM
  Entry* entry =
      findEntry(this->entries[hash_index], this->capacity[hash_index], key);
#else
  Entry* entry = findEntry(this->entries, this->capacity, key);
#endif
  if (entry->key == NULL)
    return false;

  *value = entry->value;
  return true;
}

/**
 * @brief Deletes a key-value pair from the hash table.
 *
 * Removes the entry with the specified key from the hash table by placing a
 * tombstone in its place. Does not actually free the memory associated with
 * the key or value.
 *
 * @param key The key to be deleted.
 * @return `true` if the key was found and deleted, `false` otherwise.
 */
bool Table::tableDelete(ObjString* key)
{
#ifdef ENABLE_MP
  if (this->EntriesWorkList.getLength() > 0) {
    applyWorklist();
  }
#endif

  if (this->count == 0)
    return false;

#ifdef ENABLE_MTHM
  auto hash_index = key->hash % (THREAD_COUNT - 1);

  Entry* entry =
      findEntry(this->entries[hash_index], this->capacity[hash_index], key);
#else
  Entry* entry = findEntry(this->entries, this->capacity, key);
#endif

  if (entry->key == NULL)
    return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

/**
 * @brief Finds a string in the hash table.
 *
 * Searches the hash table for a string with the given characters, length, and
 * hash value. Uses linear probing to handle collisions.
 *
 * @param chars The characters of the string to find.
 * @param length The length of the string.
 * @param hash The pre-calculated hash value of the string.
 * @return A pointer to the found string object, or NULL if not found.
 */

#ifdef ENABLE_MTHM
ObjString* Table::tableFindString(const char* chars,
                                  int length,
                                  uint32_t hash,
                                  uint32_t hash2)
#else
ObjString* Table::tableFindString(const char* chars,
                                  int length,
                                  uint32_t hash,
                                  uint32_t hash2)
#endif
{
  if (this->count == 0)
    return NULL;

#ifdef ENABLE_MTHM
  uint32_t map = hash % (THREAD_COUNT);
  uint32_t index = hash % (this->capacity[map]);
  auto list = this->entries[map];
  // printf("hashed index %d \n", index);
  for (;;) {
    Entry* entry = &list[index];
    index = (index + 1) % (this->capacity[map]);
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value))
        return NULL;
    } else if (entry->key->length == length && entry->key->hash == hash
               && memcmp(entry->key->chars, chars, length) == 0)
    {
      // We found it.
      return entry->key;
    }
  }

#else

  uint32_t index = hash & (this->capacity - 1);

  for (;;) {
    Entry* entry = &this->entries[index];
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value))
        return NULL;
    } else if (entry->key->length == length && entry->key->hash == hash
               && memcmp(entry->key->chars, chars, length) == 0)
    {
      // We found it.
      return entry->key;
    }

    index = (index + 1) & (this->capacity - 1);
  }
#endif
}

/**
 * @brief Marks all objects in the table as reachable.
 *
 * Iterates over all entries in the hash table and marks both the key and
 * value objects as reachable for garbage collection.
 */
void Table::markTable()
{
  for (int j = 0; j < THREAD_COUNT; j++) {
    for (int i = 0; i < this->capacity[j]; i++) {
      Entry* entry = &this->entries[j][i];
      markObject((Obj*)entry->key);
      markValue(entry->value);
    }
  }
}

/**
 * @brief Removes entries with unmarked keys from the table.
 *
 * Iterates over the table and removes any entries whose keys are not marked
 * as reachable. This is typically used as part of the garbage collection
 * process.
 */
void Table::tableRemoveWhite()
{
  for (int j = 0; j < THREAD_COUNT; j++) {
    for (int i = 0; i < this->capacity[j]; i++) {
      Entry* entry = &this->entries[j][i];
      if (entry->key != NULL && !entry->key->isMarked) {
        this->tableDelete(entry->key);
      }
    }
  }
}

/**
 * @brief Copies all entries from one table to another.
 *
 * Iterates through the source table and adds each key-value pair to the
 * destination table. This function does not overwrite existing entries in the
 * destination table.
 *
 * @param from The source table to copy entries from.
 * @param to The destination table to add entries to.
 */
void tableAddAll(Table* from, Table* to)
{
  for (int j = 0; j < THREAD_COUNT; j++) {
    for (int i = 0; i < from->capacity[j]; i++) {
      Entry* entry = &from->entries[j][i];
      if (entry->key != NULL) {
        to->tableSet(entry->key, entry->value);
      }
    }
  }
}
