#ifndef cpp_lox_dispatcher
#define cpp_lox_dispatcher

#include <thread>
#include <unordered_map>

#include "vm.hpp"

class Dispatcher
{
  static Dispatcher* dispatcher;
  std::unordered_map<size_t, int> id_to_vm;
  VM vm_pool[32];
  VM main_thread;
  Dispatcher();
  void initDispatcher();
  void freeDispatcher();
  int findFreeVM();

public:
  static Dispatcher* getDispatcher();
  VM* getVM();
  VM* dispatchThread(VM* parent);  // Sets new VM loop with this
  // VM* getVMByID();

  void freeVM();
};

#endif