#include "channel.hpp"

#include "lockmanager.hpp"

void ThreadChannel::initChannel()
{
  auto lockmanager = LockManager::getLockManager();
  this->closed = false;
  this->mutex_id = lockmanager->create_mutex();
  this->buffer = Queue<Value>();
}

ThreadChannel::ThreadChannel()
{
  auto lockmanager = LockManager::getLockManager();
  this->closed = false;
  this->mutex_id = lockmanager->create_mutex();
}

ThreadChannel::~ThreadChannel()
{
  // free resources here;
}

void ThreadChannel::send(Value value)
{
  // std::unique_lock<std::mutex> lock(this->mtx);
  auto lockmanager = LockManager::getLockManager();
  lockmanager->lock_mutex(this->mutex_id);
  this->buffer.enqueue(value);
  lockmanager->unlock_mutex(this->mutex_id);
}
std::optional<Value> ThreadChannel::receive()
{
  while (!closed && buffer.isEmpty()) {
    // spin lock time
  }
  if (buffer.isEmpty()) {
    return std::nullopt;  // Channel is closed and empty
  }
  auto lockmanager = LockManager::getLockManager();
  lockmanager->lock_mutex(this->mutex_id);
  auto value = buffer.dequeue();
  lockmanager->unlock_mutex(this->mutex_id);
  return value;
}

void ThreadChannel::close()
{
  auto lockmanager = LockManager::getLockManager();
  lockmanager->lock_mutex(this->mutex_id);
  this->closed = true;
  lockmanager->unlock_mutex(this->mutex_id);
}