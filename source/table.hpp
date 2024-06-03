#ifndef clox_table_h
#define clox_table_h

#include "common.hpp"
#include "value.hpp"

class Entry
{
public:
  ObjString* key;
  Value value;
};

class Table
{
  void adjustCapacity(int capacity);

public:
  int count;
  int capacity;
  Entry* entries;
  void initTable();
  void freeTable();

  bool tableSet(ObjString* key, Value value);
  bool tableGet(ObjString* key, Value* value);
  bool tableDelete(ObjString* key);

  ObjString* tableFindString(const char* chars, int length, uint32_t hash);

  void markTable();
  void tableRemoveWhite();
};

void tableAddAll(Table* from, Table* to);

#endif