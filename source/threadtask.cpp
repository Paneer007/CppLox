#include <chrono>
#include <thread>

#include "threadtask.hpp"

#include "dispatcher.hpp"

ThreadTask::ThreadTask(int id)
{
  this->vm_id = id;
  this->result = NULL;
};

const int MAX_SLEEP_TIME = 45;

void ThreadTask::wait()
{
  auto dispatcher = Dispatcher::getDispatcher();

  auto vm = dispatcher->getVMbyId(this->vm_id);
  int sleep_time = 1;
  while (vm->state != TaskState::STATE_TERMINATED) {
    // while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    //  sleep(sleep_time);
    // printf("waiting for thread id: %d \n", this->vm_id);
    sleep_time = std::min(MAX_SLEEP_TIME, sleep_time * 2);
  }
}
Value ThreadTask::get()
{
  auto dispatcher = Dispatcher::getDispatcher();
  auto vm = dispatcher->getVMbyId(this->vm_id);
  int sleep_time = 1;
  while (vm->state != TaskState::STATE_TERMINATED) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    sleep_time = std::min(MAX_SLEEP_TIME, sleep_time * 2);
  }
  return vm->futureResultValue;
}