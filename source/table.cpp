#include "table.hpp"

#include <stdlib.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"

#ifdef ENABLE_MP
#  include <omp.h>
#  include <stdlib.h>
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
  this->count = 0;
  this->capacity = 0;
  this->entries = NULL;
#ifdef ENABLE_MP
  this->threshold_count = 8;
#endif
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

static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
  uint32_t index = key->hash & (capacity - 1);
  Entry* tombstone = NULL;
  Entry* res = NULL;
#ifdef ENABLE_MP_FIND_ENTRY

#  pragma omp parallel num_threads(4) default(none) firstprivate(index) \
      shared(tombstone, res, entries, capacity, key)
  for (int thread_id = omp_get_thread_num();;
       index = (index + 4) & (capacity - 1))
  {
    auto search_id = (index + thread_id) & (capacity - 1);
    Entry* entry = &entries[search_id];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        if (tombstone != NULL) {
          res = tombstone;
        } else {
          res = entry;
        }
#  pragma omp cancel parallel
      } else {
        // We found a tombstone
        if (tombstone == NULL)
          tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      res = entry;
#  pragma omp cancel parallel
    }
#  pragma omp cancellation point parallel
  }
  return res;
#else
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
    index = (index + 1) & (capacity - 1);
  }
#endif
}

/**
 * @brief Resizes the hash table to the specified capacity.
 *
 * Creates a new hash table with the given capacity, rehashes existing entries,
 * and replaces the old table with the new one. Handles collisions and
 * tombstones during the rehashing process.
 *
 * @param capacity The new capacity for the hash table.
 */
void Table::adjustCapacity(int capacity)
{
  Entry* new_entries = ALLOCATE<Entry>(capacity);
  for (int i = 0; i < capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = NIL_VAL;
  }
  this->count = 0;
#ifdef ENABLE_MP
  int n_items = this->capacity;
  // Parallel insert stuff
  omp_lock_t* lock = (omp_lock_t*)malloc(n_items * sizeof(omp_lock_t));
  for (int i = 0; i < n_items; i++)
    omp_init_lock(&(lock[i]));
#  pragma omp parallel for
  for (int i = 0; i < this->capacity; i++) {
    auto entry = &this->entries[i];
    auto key = entry->key;
    auto value = entry->value;
    int counter = 0;
    bool workDone = false;
    bool searchDone = false;

    while (not workDone and not searchDone and counter <= 30) {
      auto hash1 = key->hash;
      auto hash2 = key->hash2;
      auto hashedValue = (hash1 + hash2 * counter)
          & (capacity - 1);  // TODO: Keep a check for integer overflow
      if (entries[hashedValue].key == NULL) {
        omp_set_lock(&(lock[hashedValue]));
        if (entries[hashedValue].key == NULL) {
          entries[hashedValue].key = key;
          entries[hashedValue].value = value;
          this->count++;
        }
        omp_unset_lock(&(lock[hashedValue]));
      }
      counter += 1;
    }
  }

  for (int i = 0; i < n_items; i++)
    omp_destroy_lock(&(lock[i]));

  free(lock);
#else
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    if (entry->key == NULL)
      continue;

    Entry* dest = findEntry(new_entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    this->count++;
  }
#endif

  FREE_ARRAY<Entry>(this->entries, this->capacity);
  this->entries = new_entries;
  this->capacity = capacity;
}

/**
 * @brief Frees the memory allocated for the hash table.
 *
 * Deallocates the memory used by the hash table's entries array and resets the
 * table's state to an empty table.
 */
void Table::freeTable()
{
  FREE_ARRAY<Entry>(this->entries, this->capacity);
  this->initTable();
}

#ifdef ENABLE_MP
void Table::applyWorklist()
{
  int n_items = worklist.size();
  // Parallel insert stuff
  omp_lock_t* lock = (omp_lock_t*)malloc(n_items * sizeof(omp_lock_t));
  for (int i = 0; i < n_items; i++)
    omp_init_lock(&(lock[i]));
#  pragma omp parallel for
  for (int i = 0; i < worklist.size(); i++) {
    auto key = worklist[i].first;
    auto value = worklist[i].second;
    int counter = 0;
    bool workDone = false;
    bool searchDone = false;

    while (not workDone and not searchDone and counter <= 30) {
      auto hash1 = key->hash;
      auto hash2 = key->hash2;
      auto hashedValue = (hash1 + hash2 * counter)
          & (capacity - 1);  // TODO: Keep a check for integer overflow
      bool empty_entry = this->entries[hashedValue].key == NULL;
      if (this->entries[hashedValue].key == NULL) {
        omp_set_lock(&(lock[hashedValue]));
        if (this->entries[hashedValue].key == NULL) {
          this->entries[hashedValue].key = key;
          this->entries[hashedValue].value = value;
          this->count++;
        }
        omp_unset_lock(&(lock[hashedValue]));
      }
      counter += 1;
    }
  }

  for (int i = 0; i < n_items; i++)
    omp_destroy_lock(&(lock[i]));

  free(lock);
  this->worklist.clear();
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
#ifdef ENABLE_MP
  if (this->count + 1 > this->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(this->capacity);
    adjustCapacity(capacity);

    applyWorklist();

  } else {
    this->worklist.push_back({key, value});
  }

#else
  if (this->count + 1 > this->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(this->capacity);
    adjustCapacity(capacity);
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
#ifdef ENABLE_MP
  if (this->worklist.size() > 0) {
    applyWorklist();
  }
#endif

  if (this->count == 0)
    return false;

  Entry* entry = findEntry(this->entries, this->capacity, key);
  if (entry->key == NULL)
    return false;

  *value = entry->value;
  return true;
}

/**
 * @brief Deletes a key-value pair from the hash table.
 *
 * Removes the entry with the specified key from the hash table by placing a
 * tombstone in its place. Does not actually free the memory associated with the
 * key or value.
 *
 * @param key The key to be deleted.
 * @return `true` if the key was found and deleted, `false` otherwise.
 */
bool Table::tableDelete(ObjString* key)
{
#ifdef ENABLE_MP
  if (this->worklist.size() > 0) {
    applyWorklist();
  }
#endif
  if (this->count == 0)
    return false;

  // Find the entry.
  Entry* entry = findEntry(this->entries, this->capacity, key);
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
ObjString* Table::tableFindString(const char* chars,
                                  int length,
                                  uint32_t hash,
                                  uint32_t hash2 = 0)
{
#ifdef ENABLE_MP
  if (this->worklist.size() > 0) {
    applyWorklist();
  }
#endif

  if (this->count == 0)
    return NULL;

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
}

/**
 * @brief Marks all objects in the table as reachable.
 *
 * Iterates over all entries in the hash table and marks both the key and value
 * objects as reachable for garbage collection.
 */
void Table::markTable()
{
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}

/**
 * @brief Removes entries with unmarked keys from the table.
 *
 * Iterates over the table and removes any entries whose keys are not marked as
 * reachable. This is typically used as part of the garbage collection process.
 */
void Table::tableRemoveWhite()
{
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    if (entry->key != NULL && !entry->key->isMarked) {
      this->tableDelete(entry->key);
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
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      to->tableSet(entry->key, entry->value);
    }
  }
}
