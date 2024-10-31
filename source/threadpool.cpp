#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "threadpool.hpp"

inline ThreadPool::ThreadPool(int threads)
{
  this->stop = false;
  for (auto i = 0; i < threads; i++) {
    workers.emplace_back(
        [this]
        {
          while (true) {
            std::function<int()> task;
            {
              std::unique_lock<std::mutex> lock(this->queue_mutex);
              this->condition.wait(
                  lock,
                  [this] { return this->stop || !this->task_queue.empty(); });
              if (this->stop && this->task_queue.empty()) {
                return;
              }
              task = std::move(this->task_queue.front());
              this->task_queue.pop();
            }
            task();
          }
        });
  }
}

auto ThreadPool::enqueue(int (&f)(VM*, VM*, int), VM*& a, VM*& b, int& c)
    -> std::future<int>
{
  using return_type = decltype(f(a, b, c));
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<int (&)(VM*, VM*, int)>(f),
                std::forward<VM*>(a),
                std::forward<VM*>(b),
                std::forward<int>(c)));
  std::future<int> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);

    if (stop)
      throw std::runtime_error("enqueue on stopped ThreadPool");
    this->task_queue.emplace([task]() { (*task)(); });
  }
  this->condition.notify_one();
  return res;
}

inline ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    this->stop = true;
  }
  this->condition.notify_all();
  for (std::thread& worker : this->workers)
    worker.join();
}

auto ThreadPool::getTP() -> ThreadPool*
{
  return ThreadPool::threadpool;
}

ThreadPool* ThreadPool::threadpool = new ThreadPool(8);