#ifndef cpp_lox_threadtask
#define cpp_lox_threadtask

#include "value.hpp"

class ThreadTask
{
  int vm_id;

public:
  ThreadTask(int id);
  void wait();
  Value get();
};

#endif