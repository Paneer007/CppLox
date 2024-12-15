#ifndef cpp_lox_dispatcher
#define cpp_lox_dispatcher

#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "object.hpp"
#include "threadtask.hpp"
#include "vm.hpp"

#define MAX_TASK 1024

class Dispatcher
{
  std::mutex dispatcher_mutex;
  static Dispatcher* dispatcher;
  std::unordered_map<size_t, int> id_to_vm;  // Make this atomic
  std::mutex id_to_vm_mtx;
  VM vm_pool[MAX_TASK];  // Make this atomic
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
  ThreadTask asyncBegin();
  void freeVM();
  void setId(size_t thread_id, int vm_id);
  void deleteId(size_t thread_id);

  ThreadTask launchFuture();
  VM* getVMbyId(int vm_id);

  void terminateAllThreads();
  void set_active_thread(size_t thread_id);
  void free_active_thread(size_t thread_id);
  ThreadTask dispatch_loop_thread(int index, int initial_index, bool preduce);
  void initVMs();

  void flagVM(size_t vm_id);
};

#endif