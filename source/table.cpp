#include "table.hpp"

#include <stdlib.h>
#include <string.h>

#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"

#define TABLE_MAX_LOAD 0.75

void Table::initTable()
{
  this->count = 0;
  this->capacity = 0;
  this->entries = NULL;
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
  uint32_t index = key->hash & (capacity - 1);
  Entry* tombstone = NULL;
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
}

void Table::adjustCapacity(int capacity)
{
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  this->count = 0;
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    if (entry->key == NULL)
      continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    this->count++;
  }
  FREE_ARRAY(Entry, this->entries, this->capacity);
  this->entries = entries;
  this->capacity = capacity;
}

void Table::freeTable()
{
  FREE_ARRAY(Entry, this->entries, this->capacity);
  this->initTable();
}

bool Table::tableSet(ObjString* key, Value value)
{
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
}

bool Table::tableGet(ObjString* key, Value* value)
{
  if (this->count == 0)
    return false;

  Entry* entry = findEntry(this->entries, this->capacity, key);
  if (entry->key == NULL)
    return false;

  *value = entry->value;
  return true;
}

bool Table::tableDelete(ObjString* key)
{
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

ObjString* Table::tableFindString(const char* chars, int length, uint32_t hash)
{
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

void Table::markTable()
{
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}

void Table::tableRemoveWhite()
{
  for (int i = 0; i < this->capacity; i++) {
    Entry* entry = &this->entries[i];
    if (entry->key != NULL && !entry->key->isMarked) {
      this->tableDelete(entry->key);
    }
  }
}

void tableAddAll(Table* from, Table* to)
{
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      to->tableSet(entry->key, entry->value);
    }
  }
}
