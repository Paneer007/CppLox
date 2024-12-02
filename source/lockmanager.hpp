#ifndef cpp_lox_reducer
#define cpp_lox_reducer

#include <mutex>
#include <unordered_map>

#include "vm.hpp"

class LockManager
{
private:
  static LockManager* lockmanager;

  std::unordered_map<int, std::unique_ptr<std::mutex>> mutex_map;
  std::unordered_map<VM*, int> vm_map;

public:
  static LockManager* getLockManager();

  void create_mutex();
  void create_preduce_mutex(VM* vm);
  void lock_mutex();
  void lock_preduce_mutex(VM* vm);
  void unlock_mutex();
  void unlock_preduce_mutex(VM* vm);

  void destroy_mutex();
  void destroy_preduce_mutex(VM* vm);
};

#endif