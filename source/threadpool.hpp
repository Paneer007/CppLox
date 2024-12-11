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

// static void evict_thread();

class ThreadPool
{
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> task_queue;
  std::mutex queue_mutex;
  std::condition_variable condition;
  std::atomic_bool stop;
  static ThreadPool* threadpool;

public:
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  ThreadPool(int threads = std::thread::hardware_concurrency())
  {
    if (!threads)
      throw std::invalid_argument("more than zero threads expected");
    this->stop = false;
    // Creating workers
    for (auto i = 0; i < threads; i++) {
      // Spawning a thread that basically loops infinitely and
      // Checks if there a task in the queue
      // If a task exists, we dequeue it and execute it
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
              task();
            }
          });
    }

    // Thread to evict current thread of execution
    // workers.emplace_back(evict_thread);
  }

  static auto getTP() -> ThreadPool* { return ThreadPool::threadpool; }

  template<class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>
  {
    // Return type of the packaged tasks
    using packaged_task_t =
        std::packaged_task<typename std::result_of<F(Args...)>::type()>;

    // Storing args and function as a parameter
    std::shared_ptr<packaged_task_t> task(new packaged_task_t(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)));

    // Enqueue the task and return the future object
    auto res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(this->queue_mutex);
      this->task_queue.emplace([task]() { (*task)(); });
    }
    // update the mutex conditional variable
    this->condition.notify_one();
    return res;
  }

  ~ThreadPool()
  {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      this->stop = true;
    }
    this->condition.notify_all();
    for (std::thread& worker : this->workers)
      worker.join();
  }
};

// static void evict_thread()
// {
//   auto sleep_time = 1000;
//   while (true) {
//     printf("This is going great. \n");
//     std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
//   }
// }

#endif