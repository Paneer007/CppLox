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

#include "dispatcher.hpp"
#include "vm.hpp"

#define MAX_TP 4

static void evict_thread();

class ThreadPool
{
  std::vector<std::thread> workers;
  std::condition_variable condition;
  std::atomic_bool stop;
  static ThreadPool* threadpool;

public:
  std::mutex queue_mutex;
  std::vector<size_t> processing;
  std::queue<std::function<void()>> task_queue;

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
          [this, i]
          {
            while (true) {
              std::function<void()> task;
              size_t thread_id = -1;
              {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(
                    lock,
                    [this] { return this->stop || !this->task_queue.empty(); });
                if (this->stop && this->task_queue.empty()) {
                  return;
                }
                task = std::move(this->task_queue.front());
                thread_id =
                    std::hash<std::thread::id> {}(std::this_thread::get_id());
                // printf("appended thread id %d\n", thread_id);
                this->task_queue.pop();
                this->processing.push_back(thread_id);
                // for (auto& x : this->processing) {
                // printf("thread id is : %d \n", x);
                // }
              }
              task();
              if (thread_id != -1) {
                // printf("deleted thread id: %d \n", thread_id);
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                // for (auto& x : this->processing) {
                // printf("pre deleting thread id is : %d \n", x);
                // }
                this->processing.erase(std::remove(this->processing.begin(),
                                                   this->processing.end(),
                                                   thread_id),
                                       this->processing.end());

                // this->processing.erase(std::find(this->processing.begin(),
                //                                  this->processing.end(),
                //                                  thread_id));
                // for (auto& x : this->processing) {
                // printf("deleted thread id is : %d \n", x);
                // }
              }
            }
          });
    }

#ifdef ROUND_ROBIN
    // Thread to evict current thread of execution
    workers.emplace_back(evict_thread);
#endif
  }

  static auto getTP() -> ThreadPool*
  {
    if (ThreadPool::threadpool == NULL) {
      ThreadPool::threadpool = new ThreadPool(MAX_TP);
    }
    return ThreadPool::threadpool;
  }

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

static void evict_thread()
{
  auto sleep_time = 10;
  auto dispatcher = Dispatcher::getDispatcher();
  while (true) {
    // printf("good morning waking up from sleep \n");
    auto tp = ThreadPool::getTP();
    tp->queue_mutex.lock();
    // printf("count of stuff: %d \n", tp->processing.size());
    auto condition_one = tp->processing.size() == MAX_TP;
    auto condition_two = tp->task_queue.size() != 0;
    if (!(condition_one and condition_two)) {
      tp->queue_mutex.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
      continue;
    }
    // printf("Thread Pool processing size: %d \n", tp->processing.size());
    auto thread_id = tp->processing.front();
    // tp->processing.pop_back();
    dispatcher->flagVM(thread_id);
    tp->queue_mutex.unlock();
    // printf("done with this going back to sleep \n");
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
  }
}

#endif