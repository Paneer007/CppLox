#ifndef cpplox_memory_space
#define cpplox_memory_space

#include <thread>

#include "object.hpp"

class VM;

typedef enum
{
  NURSERY,
  SURVIVOR,
  TENURED
} Generation;

class MemorySpace;
class MemoryGeneration;

class MemoryGeneration
{
  Generation gen;
  MemorySpace* ms;
  int nextSweep;

  inline int increaseCapacity();

public:
  Obj* head;
  Obj* tail;
  int bytesAllocated;
  void initMG(Generation gen, MemorySpace* ms);
  void addObject(Obj* newNode);
  bool updateStorage(int size);
  bool resetStorage();
  void sweep();
};

class MemorySpace
{
  MemoryGeneration nursery;
  MemoryGeneration survivor;
  MemoryGeneration tenured;
  std::vector<std::thread> memoryThread;
  bool startMarking;
  VM* vm;

  void doMark();
  bool moveGenerations(MemoryGeneration& A, MemoryGeneration& B);
  void checkForTenuredGeneration();

public:
  MemorySpace();
  void initMS(VM* vm);
  void startMarkingThread();
  void waitMarkingThread();
  void freeMarkingThread();
  void resumeMarkingThread();
  void forcedMark();
  void updateNurseryStorage(int size);
  void addObjectToNursery(Obj* newNode);
};

#endif