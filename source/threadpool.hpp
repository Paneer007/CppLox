#ifndef clox_thread_pool
#define clox_thread_pool

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "vm.hpp"

class ThreadPool
{
  std::vector<std::thread> workers;
  std::queue<std::function<int()>> task_queue;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;

public:
  ThreadPool(int threads);
  static ThreadPool* threadpool;

  static auto getTP() -> ThreadPool*;

  auto enqueue(int (&f)(VM*, VM*, int) , VM*&, VM*&, int&) -> std::future<int>;

  ~ThreadPool();
};

#endif