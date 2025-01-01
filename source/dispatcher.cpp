#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "dispatcher.hpp"

#include <unistd.h>

#include "debug.hpp"
#include "object.hpp"
#include "threadpool.hpp"

static inline bool VMExecution(VM* childVM);
static int voidVMExecution(VM* childVM, int vm_id);
static int futureTask(VM* childVM, int vm_id, bool isFuture);

Dispatcher::Dispatcher()
{
  this->initDispatcher();
}

void Dispatcher::setId(size_t thread_id, int vm_id)
{
  std::lock_guard<std::mutex> lock(this->id_to_vm_mtx);
  this->id_to_vm[thread_id] = vm_id;
}

void Dispatcher::initDispatcher()
{
  this->id_to_vm = std::unordered_map<size_t, int>();
  this->thread_arr = std::vector<size_t>();
}

void Dispatcher::initVMs()
{
  for (int i = 0; i < MAX_TASK; i++) {
    this->vm_pool[i].initVM();
  }
}

void Dispatcher::freeDispatcher()
{
  for (int i = 0; i < MAX_TASK; i++) {
    this->vm_pool[i].freeVM();
  }
  this->id_to_vm.clear();
}

int Dispatcher::findFreeVM()
{
  std::lock_guard<std::mutex> lock(this->vm_pool_mtx);
  for (int i = 0; i < MAX_TASK; i++) {
    if (this->vm_pool[i].assigned == false) {
      // printf("vm_id: %d is assigned \n", i);
      this->vm_pool[i].assigned = true;
      return i;
    }
  }
  return -1;
}

VM* Dispatcher::getVM()
{
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  if (this->id_to_vm.find(thread_id) == this->id_to_vm.end()) {
    printf("Accessing thread that doesn't exist in the Dispatcher \n");
    exit(0);
  }
  auto vm_id = this->id_to_vm[thread_id];
  return &this->vm_pool[vm_id];
}

VM* Dispatcher::dispatchThread(VM* parent)
{
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  if (this->id_to_vm.find(thread_id) != this->id_to_vm.end()) {
    printf("Creating thread that already exist in the Dispatcher");
    exit(0);
  }
  auto vm_id = this->findFreeVM();  // Handle full VM error
  while (vm_id == -1) {
    sleep(1);
    vm_id = this->findFreeVM();
  }

  dispatcher->set_active_thread(thread_id);

  auto childVM = &this->vm_pool[vm_id];
  this->setId(thread_id, vm_id);
  childVM->copyParent(parent);

  return childVM;
}

void Dispatcher::freeVM()
{
  std::lock_guard<std::mutex> lock1(this->id_to_vm_mtx);
  std::lock_guard<std::mutex> lock2(this->vm_pool_mtx);

  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  if (this->id_to_vm.find(thread_id) == this->id_to_vm.end()) {
    printf("Accessing thread that doesn't exist in the Dispatcher");

    exit(0);
  }
  auto vm_id = this->id_to_vm[thread_id];
  auto vm = &this->vm_pool[vm_id];
  vm->freeVM();
  this->id_to_vm.erase(thread_id);
}

Dispatcher* Dispatcher::getDispatcher()
{
  return Dispatcher::dispatcher;
}

ThreadTask Dispatcher::asyncBegin()
{
  auto parent_vm = this->getVM();
  auto free_vm_index = this->findFreeVM();
  auto childVM = &this->vm_pool[free_vm_index];
  auto thread_task = ThreadTask(free_vm_index);
  // auto start = std::chrono::high_resolution_clock::now();
  childVM->copyParent(parent_vm);
  // auto end = std::chrono::high_resolution_clock::now();
  // auto duration =
  // std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  // std::cout << "Time taken to create a VM: " << duration.count() <<
  // std::endl;

  auto frame = &childVM->frames[childVM->frameCount - 1];
  frame->ip += 2;  // Skip jump
  auto tp = ThreadPool::getTP();
  tp->enqueue(voidVMExecution, childVM, free_vm_index);
  return thread_task;
}

ThreadTask Dispatcher::launchFuture()
{
  auto parent_vm = this->getVM();
  auto free_vm_index = this->findFreeVM();
  auto childVM = &this->vm_pool[free_vm_index];
  auto thread_task = ThreadTask(free_vm_index);

  childVM->isFuture = true;
  // auto start = std::chrono::high_resolution_clock::now();
  childVM->copyParent(parent_vm);
  // auto end = std::chrono::high_resolution_clock::now();
  // auto duration =
  // std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  // std::cout << "Time taken to run a VM" << duration.count() <<
  // std::endl;
  auto frame = &childVM->frames[childVM->frameCount - 1];
  frame->ip += 3;  // Skip call
  // Launch Future
  auto tp = ThreadPool::getTP();
  tp->enqueue(futureTask, childVM, free_vm_index, true);
  return thread_task;
}

VM* Dispatcher::getVMbyId(int vm_id)
{
  return &this->vm_pool[vm_id];
}

void Dispatcher::set_active_thread(size_t thread_id)
{
  std::unique_lock<std::mutex> lock(this->dispatcher_mutex);
  this->thread_arr.push_back(thread_id);
}

void Dispatcher::free_active_thread(size_t thread_id)
{
  std::unique_lock<std::mutex> lock(this->dispatcher_mutex);
  auto it =
      std::find(this->thread_arr.begin(), this->thread_arr.end(), thread_id);

  if (it != this->thread_arr.end()) {
    this->thread_arr.erase(it);
  }
}

void Dispatcher::terminateAllThreads()
{
  for (auto& x : this->thread_arr) {
    auto vm = &vm_pool[id_to_vm[x]];
    vm->threadFailure = true;
  }
}

ThreadTask Dispatcher::dispatch_loop_thread(int index,
                                            int initial_index,
                                            bool preduce)
{
  auto parent_vm = this->getVM();
  auto free_vm_index = this->findFreeVM();
  auto childVM = &this->vm_pool[free_vm_index];
  auto thread_task = ThreadTask(free_vm_index);

  // auto start = std::chrono::high_resolution_clock::now();
  childVM->copyParent(parent_vm);
  // auto end = std::chrono::high_resolution_clock::now();
  // auto duration =
  //     std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  // std::cout << "Time taken to run a VM: " << duration.count() << std::endl;

  if (preduce) {
    // Copying last stack elements:
    int diff = parent_vm->stackTop - parent_vm->stack;
    std::copy(parent_vm->stack + diff - 7,
              parent_vm->stackTop,
              childVM->stack + diff - 7);
    childVM->parentLastStackElement -= 4;
  } else {
    // Copying last stack elements:
    int diff = parent_vm->stackTop - parent_vm->stack;
    std::copy(parent_vm->stack + diff - 3,
              parent_vm->stackTop,
              childVM->stack + diff - 3);
    childVM->parentLastStackElement -= 3;
  }

  auto frame = &childVM->frames[childVM->frameCount - 1];
  // *(childVM->stackTop - 2) = NUMBER_VAL(index);  // Test this
  *(childVM->stackTop - initial_index) =
      NUMBER_VAL(static_cast<double>(index));  // Test this
  frame->ip += 2;  // Skip jump statement

  auto tp = ThreadPool::getTP();
  tp->enqueue(futureTask, childVM, free_vm_index, false);

  return thread_task;
}

void Dispatcher::flagVM(size_t thread_id)
{
  std::unique_lock<std::mutex> lock(this->dispatcher_mutex);
  auto vm_id = id_to_vm[thread_id];
  // printf("vm_id: %d is flagged \n", vm_id);
  auto vm = &this->vm_pool[vm_id];
  vm->evictThread = true;
}

void Dispatcher::deleteId(size_t thread_id)
{
  std::unique_lock<std::mutex> lock(this->dispatcher_mutex);

  auto it =
      std::find(this->thread_arr.begin(), this->thread_arr.end(), thread_id);

  if (it != this->thread_arr.end()) {
    this->thread_arr.erase(it);
  }
}

void Dispatcher::setopenMPVM(VM* vm)
{
  std::unique_lock<std::mutex> lock(this->dispatcher_mutex);
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  this->openMP_vm[thread_id] = vm;
}

VM* Dispatcher::getopenMPVM()
{
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  return this->openMP_vm[thread_id];
}

static inline bool VMExecution(VM* childVM)
{
  childVM->state = TaskState::STATE_RUNNING;  // Set VM to be running
  auto res = childVM->run();
  switch (res) {
    case INTERPRET_RUNTIME_ERROR:
      childVM->state = TaskState::STATE_TERMINATED;  // Terminate VM and return
                                                     // true for error
      return true;
      break;
    case INTERPRET_EVICT:
      childVM->state = TaskState::STATE_READY;  // Set VM to be ready state
      return false;
      break;
    case INTERPRET_OK:
      childVM->state = TaskState::STATE_TERMINATED;  // Terminate VM and return
                                                     // false for error
      return false;
    default:
      printf("Unexpected error \n");
      exit(0);
  }
  return false;
}

static int voidVMExecution(VM* childVM, int vm_id)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  dispatcher->setId(thread_id, vm_id);
  dispatcher->set_active_thread(thread_id);
  childVM->evictThread = false;

  if (VMExecution(childVM)) {
    dispatcher->terminateAllThreads();
    printf("unexpected error \n");
    exit(0);
    return 1;
  }

  switch (childVM->state) {
    case TaskState::STATE_NEW:
    case TaskState::STATE_RUNNING:
      // All are unexpected states
      exit(0);
      break;
    case TaskState::STATE_TERMINATED:
      dispatcher->free_active_thread(thread_id);
      dispatcher->deleteId(thread_id);
      return 0;
      break;
    case TaskState::STATE_READY:
      auto tp = ThreadPool::getTP();
      dispatcher->free_active_thread(thread_id);
      dispatcher->deleteId(thread_id);
      tp->enqueue(voidVMExecution, childVM, vm_id);
      // printf("done enqueing thread \n");
      return 0;
      break;
  }

  return 0;
}

static int futureTask(VM* childVM, int vm_id, bool isFuture)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  dispatcher->setId(thread_id, vm_id);
  dispatcher->set_active_thread(thread_id);
  childVM->evictThread = false;
  childVM->isFuture = isFuture;

  if (VMExecution(childVM)) {
    dispatcher->terminateAllThreads();
    exit(0);
  }

  switch (childVM->state) {
    case TaskState::STATE_NEW:
    case TaskState::STATE_RUNNING:
      // All are unexpected states
      break;
    case TaskState::STATE_TERMINATED:
      childVM->pop();
      dispatcher->free_active_thread(thread_id);
      dispatcher->deleteId(thread_id);
      // printf("terminated VM ID: %d \n", vm_id);
      childVM->isFuture = false;
      // printf("done element \n");
      return 0;
    case TaskState::STATE_READY:
      auto tp = ThreadPool::getTP();
      dispatcher->free_active_thread(thread_id);
      dispatcher->deleteId(thread_id);
      tp->enqueue(futureTask, childVM, vm_id, isFuture);
      return 0;
  }
  return 0;
}

Dispatcher* Dispatcher::dispatcher = new Dispatcher;