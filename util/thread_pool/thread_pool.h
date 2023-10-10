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

#include "logger/log.h"

class ThreadPool {
 public:
  ThreadPool(const std::string& threadpool_name, const uint32_t thread_cnt);
  ~ThreadPool();

 public:
  /**
   * @brief 将任务提交到线程池并返回一个 std::future 对象
   *
   * @tparam F
   * @tparam Args
   * @param f
   * @param args
   * @return std::future<typename std::result_of<F(Args...)>::type>
   */
  template <typename F, typename... Args>
  auto Post(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

 private:
  // 线程池
  std::string threadpool_name_;
  std::vector<std::thread> workers_;
  std::atomic<uint32_t> idle_worker_cnt = {0};
  uint32_t total_worker_cnt_ = {0};

  // 任务队列
  std::queue<std::function<void()>> tasks_;
  std::mutex tasks_mutex_;
  std::condition_variable tasks_cv_;

  // 线程池是否在运行中
  std::atomic<bool> is_running_ = {false};
};

template <class F, class... Args>
auto ThreadPool::Post(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
  using ReturnType = typename std::result_of<F(Args...)>::type;
  CHECK(is_running_) << "Threadpool [" << threadpool_name_ << "] has been stopped!";

  auto task =
      std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  std::future<ReturnType> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    tasks_.emplace([task]() {
      (*task)();
    });
    tasks_cv_.notify_one();
  }
  return res;
}
