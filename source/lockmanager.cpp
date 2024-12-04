#include <stdexcept>

#include "lockmanager.hpp"

LockManager* LockManager::getLockManager()
{
  return LockManager::lockmanager;
}

int LockManager::create_mutex()
{
  auto mutex_id = rand();
  this->mutex_map[mutex_id] = std::make_unique<std::mutex>();
  return mutex_id;
}

void LockManager::create_preduce_mutex(VM* vm)
{
  if (this->vm_map.find(vm) != this->vm_map.end()) {
    throw std::runtime_error("Lock already exists for this VM");
  }
  auto mutex_id = rand();
  this->vm_map[vm] = mutex_id;
  this->mutex_map[mutex_id] = std::make_unique<std::mutex>();
}

void LockManager::lock_mutex(int id)
{
  this->mutex_map[id]->lock();
}
void LockManager::lock_preduce_mutex(VM* vm)
{
  this->mutex_map[this->vm_map[vm]]->lock();
}

void LockManager::unlock_mutex(int id)
{
  this->mutex_map[id]->unlock();
}

void LockManager::unlock_preduce_mutex(VM* vm)
{
  this->mutex_map[this->vm_map[vm]]->unlock();
}

void LockManager::destroy_mutex(int id) {}

void LockManager::destroy_preduce_mutex(VM* vm) {}

LockManager* LockManager::lockmanager = new LockManager;