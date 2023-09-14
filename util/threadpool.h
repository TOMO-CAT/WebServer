#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace util {

class ThreadPool {
 public:
  ThreadPool(const std::string& threadpool_name, const uint32_t thread_cnt);
  template <typename F, typename... Args>
  auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
  ~ThreadPool();

 private:
  // 线程池
  std::vector<std::thread> workers_;
  std::atomic<uint32_t> idle_worker_cnt = {0};
  uint32_t total_worker_cnt_ = {0};

  // 任务队列
  std::queue<std::function<void()>> tasks_;
  std::mutex tasks_mutex_;
  std::condition_variable tasks_cv_;

  // 线程池是否在运行中
  std::atomic<bool> is_running_ = {false};

  // 是否打印 debug 信息
  std::atomic<bool> enable_check_ = {false};
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads) : stop_(false) {
  for (size_t i = 0; i < threads; ++i) {
    workers_.emplace_back([this] {
      for (;;) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(this->queue_mutex_);
          this->condition_.wait(lock, [this] {
            return this->stop_ || !this->tasks_.empty();
          });
          if (this->stop_ && this->tasks_.empty()) {
            return;
          }
          task = std::move(this->tasks_.front());
          this->tasks_.pop();
        }
        task();
      }
    });
  }
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task =
      std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // don't allow enqueueing after stopping the pool
    if (stop_) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }

    tasks_.emplace([task]() {
      (*task)();
    });
  }
  condition_.notify_one();
  return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

}  // namespace util
