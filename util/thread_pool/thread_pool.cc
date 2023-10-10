#include "util/thread_pool/thread_pool.h"

#include <future>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "logger/log.h"

ThreadPool::ThreadPool(const std::string& threadpool_name, const uint32_t thread_cnt)
    : threadpool_name_(threadpool_name) {
  is_running_.store(true);
  for (uint32_t i = 0; i < thread_cnt; ++i) {
    workers_.emplace_back([this, i]() {
      std::string thread_name = threadpool_name_ + "_" + std::to_string(i);
      ::pthread_setname_np(::pthread_self(), thread_name.c_str());
      while (is_running_) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(tasks_mutex_);
          if (!is_running_ && tasks_.empty()) {
            return;
          }
          tasks_cv_.wait(lock, [this]() {
            return !is_running_ || !tasks_.empty();
          });
          if (!is_running_ && tasks_.empty()) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }
        --idle_worker_cnt;
        task();
        ++idle_worker_cnt;
      }
    });
    ++idle_worker_cnt;
  }
  LOG_INFO << "Start threadpool [" << threadpool_name << "] with [" << thread_cnt << "] threads successfully!";
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(tasks_mutex_);
    is_running_.store(false);
    tasks_cv_.notify_all();
  }
  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  LOG_INFO << "Stop threadpool [" << threadpool_name_ << "] successfully!";
}
