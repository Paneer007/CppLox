#include <iostream>
#include <thread>
#include <unordered_map>

#include "dispatcher.hpp"

Dispatcher::Dispatcher()
{
  this->initDispatcher();
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
  this->main_thread.freeVM();
  this->id_to_vm.clear();
}

int Dispatcher::findFreeVM()
{
  for (int i = 0; i < 32; i++) {
    if (this->vm_pool[i].assigned == false) {
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
  auto vm_id = this->findFreeVM();
  auto childVM = &this->vm_pool[vm_id];
  this->id_to_vm[thread_id] = vm_id;
  childVM->copyParent(parent);
  return childVM;
}

void Dispatcher::freeVM()
{
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  if (this->id_to_vm.find(thread_id) == this->id_to_vm.end()) {
    printf("Accessing thread that doesn't exist in the Dispatcher");
    exit(0);
  }
  auto vm_id = this->id_to_vm[thread_id];
  auto vm = &this->vm_pool[vm_id];
  vm->freeVM();
}

Dispatcher* Dispatcher::getDispatcher()
{
  return Dispatcher::dispatcher;
}

Dispatcher* Dispatcher::dispatcher = new Dispatcher;