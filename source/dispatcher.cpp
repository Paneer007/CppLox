#include <iostream>
#include <thread>
#include <unordered_map>

#include "dispatcher.hpp"

#include "debug.hpp"

static void childMain(VM* parent, VM* childVM, int vm_id)
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
  dispatcher->setId(thread_id, vm_id);

  auto frame = &childVM->frames[childVM->frameCount - 1];
  childVM->run();
}

Dispatcher::Dispatcher()
{
  this->initDispatcher();
}

void Dispatcher::setId(size_t thread_id, int vm_id)
{
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
    vm_id = this->findFreeVM();
  }

  auto childVM = &this->vm_pool[vm_id];
  this->setId(thread_id, vm_id);
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

Dispatcher* Dispatcher::dispatcher = new Dispatcher;