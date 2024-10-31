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
  std::queue<std::function<void()>> task_queue;
  std::mutex queue_mutex;
  std::condition_variable condition;
  std::atomic_bool stop;

public:
  ThreadPool(int threads);
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;
  static ThreadPool* threadpool;

  static auto getTP() -> ThreadPool*;

  template<class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>
  {
    using packaged_task_t =
        std::packaged_task<typename std::result_of<F(Args...)>::type()>;

    std::shared_ptr<packaged_task_t> task(new packaged_task_t(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
    auto res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(this->queue_mutex);
      this->task_queue.emplace([task]() { (*task)(); });
    }
    this->condition.notify_one();
    return res;
  }

  ~ThreadPool();
};

#endif