#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "threadpool.hpp"

inline ThreadPool::ThreadPool(int threads = std::thread::hardware_concurrency())
{
  if (!threads)
    throw std::invalid_argument("more than zero threads expected");
  this->stop = false;
  for (auto i = 0; i < threads; i++) {
    workers.emplace_back(
        [this]
        {
          while (true) {
            std::function<void()> task;
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
            printf("\n Executing task \n");
            task();
            printf("\n Done task \n");
          }
        });
  }
}

// template<class F, class... Args>
// auto ThreadPool::enqueue(F&& f, Args&&... args)
//     -> std::future<typename std::result_of<F(Args...)>::type>
// {
//   using packaged_task_t =
//       std::packaged_task<typename std::result_of<F(Args...)>::type()>;

//   std::shared_ptr<packaged_task_t> task(new packaged_task_t(
//       std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
//   auto res = task->get_future();
//   {
//     std::unique_lock<std::mutex> lock(this->queue_mutex);
//     this->task_queue.emplace([task]() { (*task)(); });
//   }
//   this->condition.notify_one();
//   return res;
// }

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

ThreadPool* ThreadPool::threadpool = new ThreadPool();