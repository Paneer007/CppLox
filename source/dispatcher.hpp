#ifndef cpp_lox_dispatcher
#define cpp_lox_dispatcher

#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "object.hpp"
#include "vm.hpp"

class Dispatcher
{
  static Dispatcher* dispatcher;

  std::unordered_map<size_t, int> id_to_vm;  // Make this atomic
  std::mutex id_to_vm_mtx;
  VM vm_pool[32];  // Make this atomic
  std::mutex vm_pool_mtx;
  std::vector<size_t> thread_arr;
  Dispatcher();
  void initDispatcher();
  void freeDispatcher();
  int findFreeVM();

public:
  static Dispatcher* getDispatcher();
  VM* getVM();
  VM* dispatchThread(VM* parent);  // Sets new VM loop with this
  void asyncBegin(std::list<std::future<int>>& futures);
  void freeVM();
  void setId(size_t thread_id, int vm_id);

  int launchFuture();
  VM* getVMbyId(int vm_id);

  void terminateAllThreads();
  void set_active_thread(size_t thread_id);
  void free_active_thread(size_t thread_id);
  void dispatch_loop_thread(int index,
                            std::list<std::future<int>>& futures,
                            int initial_index);

  void initVMs();
};

#endif