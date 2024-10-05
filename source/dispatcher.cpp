#include <algorithm>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "dispatcher.hpp"

#include <unistd.h>

#include "debug.hpp"
#include "object.hpp"

static void childMain(VM* parent, VM* childVM, int vm_id)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  dispatcher->setId(thread_id, vm_id);
  dispatcher->set_active_thread(thread_id);
  auto frame = &childVM->frames[childVM->frameCount - 1];

  auto res = childVM->run();
  if (res == INTERPRET_RUNTIME_ERROR) {
    dispatcher->terminateAllThreads();
    exit(0);
  }
  dispatcher->free_active_thread(thread_id);
}

static void futureTask(VM* parent, VM* childVM, int vm_id)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  dispatcher->setId(thread_id, vm_id);
  dispatcher->set_active_thread(thread_id);
  childVM->isFuture = true;
  auto vm_res = childVM->run();
  if (vm_res == INTERPRET_RUNTIME_ERROR) {
    dispatcher->terminateAllThreads();
  }
  auto res = childVM->pop();
  dispatcher->free_active_thread(thread_id);
  childVM->isFuture = false;
}

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
}

void Dispatcher::freeDispatcher()
{
  for (int i = 0; i < 32; i++) {
    this->vm_pool[i].freeVM();
  }
  this->id_to_vm.clear();
}

int Dispatcher::findFreeVM()
{
  std::lock_guard<std::mutex> lock(this->vm_pool_mtx);
  for (int i = 0; i < 32; i++) {
    if (this->vm_pool[i].assigned == false) {
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

std::thread Dispatcher::asyncBegin()
{
  auto parent_vm = this->getVM();
  auto free_vm_index = this->findFreeVM();
  auto childVM = &this->vm_pool[free_vm_index];
  childVM->copyParent(parent_vm);
  auto frame = &childVM->frames[childVM->frameCount - 1];
  frame->ip += 2;  // Skip jump

  std::thread child_obj(childMain, parent_vm, childVM, free_vm_index);
  return child_obj;
}

int Dispatcher::launchFuture()
{
  auto parent_vm = this->getVM();
  auto free_vm_index = this->findFreeVM();
  auto childVM = &this->vm_pool[free_vm_index];
  childVM->isFuture = true;
  childVM->copyParent(parent_vm);
  auto frame = &childVM->frames[childVM->frameCount - 1];
  frame->ip += 3;  // Skip call
  // Launch Future
  futureTask(parent_vm, childVM, free_vm_index);

  return free_vm_index;
}

VM* Dispatcher::getVMbyId(int vm_id)
{
  return &this->vm_pool[vm_id];
}

void Dispatcher::set_active_thread(size_t thread_id)
{
  this->thread_arr.push_back(thread_id);
}

void Dispatcher::free_active_thread(size_t thread_id)
{
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

Dispatcher* Dispatcher::dispatcher = new Dispatcher;