#ifndef clox_table_h
#define clox_table_h

#include <utility>

#include "common.hpp"
#include "value.hpp"
#include "vector"

#define THREAD_COUNT 16
/**
 * @brief Represents an entry in a hash table.
 *
 * Stores a key-value pair. The key is an `ObjString` and the value is a generic
 * `Value`.
 */
class Entry
{
public:
  /**
   * @brief The key of the entry.
   */
  ObjString* key;

  /**
   * @brief  The value associated with the key.
   */
  Value value;
};

class MultiDimWorkList
{
public:
  int count[16];
  int capacity[16];

  Entry** entries;

  void adjustCapacity(int capacity);
  void initList();
  void writeList(ObjString* key, Value value);
  void clearList();
  Entry* getElement(int map, int index);
};

/**
 * @brief Represents a hash table for storing key-value pairs.
 *
 * Provides methods for inserting, retrieving, deleting, and managing entries in
 * the hash table. Implements resizing, rehashing, and garbage collection
 * support.
 */
class Table
{
  /**
   * @brief Resizes the hash table to the specified capacity.
   *
   * Creates a new hash table with the given capacity, rehashes existing
   * entries, and replaces the old table with the new one. Handles collisions
   * and tombstones during the rehashing process.
   *
   * @param capacity The new capacity for the hash table.
   */
  void adjustCapacity(int capacity, int index);

  void applyWorklist();

  void clearWorklist();

public:
  /**
   * @brief The number of key-value pairs currently stored in the table.
   */
  // int count;

  // /**
  //  * @brief The maximum capacity of the table.
  //  */
  // int capacity;

  // /**
  //  * @brief An array of entries storing key-value pairs.
  //  */
  // Entry* entries;

#ifdef ENABLE_MTHM
  int count[16];
  int capacity[16];

  std::vector<std::vector<std::pair<ObjString*, Value>>> worklist;
  Entry** entries;
#endif
  /**
   * @brief Represents a hash table for storing key-value pairs.
   *
   * Provides methods for inserting, retrieving, deleting, and managing entries
   * in the hash table. Implements resizing, rehashing, and garbage collection
   * support.
   */

  /**
   * @brief Initializes an empty hash table.
   */
  void initTable();

  /**
   * @brief Frees the memory allocated for the hash table.
   */
  void freeTable();

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
  bool tableSet(ObjString* key, Value value);

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
  bool tableGet(ObjString* key, Value* value);

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
  bool tableDelete(ObjString* key);

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
  ObjString* tableFindString(const char* chars,
                             int length,
                             uint32_t hash,
                             uint32_t hash2);

  /**
   * @brief Marks all objects in the table as reachable.
   *
   * Iterates over all entries in the hash table and marks both the key and
   * value objects as reachable for garbage collection.
   */
  void markTable();

  /**
   * @brief Removes entries with unmarked keys from the table.
   *
   * Iterates over the table and removes any entries whose keys are not marked
   * as reachable. This is typically used as part of the garbage collection
   * process.
   */
  void tableRemoveWhite();

  void tableBulkSearch();
};

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
void tableAddAll(Table* from, Table* to);

#endif