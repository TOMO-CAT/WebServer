#pragma once

#include <future>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "logger/logger.h"
#include "util/thread_pool/thread_pool_util.h"

namespace auto_pilot {
namespace util {

class ThreadPool {
 public:
  /**
   * @brief ThreadPool 构造函数
   *
   * @param thread_name 线程池名称
   * @param number 线程数量
   * @param thread_pool_index 线程池索引
   * @param next_thread_pool_level 下一个层级线程池的 LEVEL, 最顶层线程池的 LEVEL 为 0
   */
  ThreadPool(const std::string& thread_name, const uint32_t number, const uint32_t thread_pool_index,
             const uint32_t next_thread_pool_level);

  ~ThreadPool();

 public:
  void Start();
  void Stop();
  void Wait();
  void EnableCheck();
  void DisableCheck();
  uint32_t idl_thread_num() const;
  uint32_t size() const;

 public:
  /**
   * @brief 将函数应用于输入元素的范围并将结果存储在输出容器中
   *
   */
  template <typename InputIter, typename OutputIter, typename F>
  OutputIter Transform(InputIter begin, InputIter end, OutputIter out, F&& f) {
    typedef std::future<decltype(f(*begin))> Future;
    std::vector<Future> futures;
    for (auto iter = begin; iter != end; ++iter) {
      auto& element = *iter;
      // we need wait for result, so & is fine here
      futures.emplace_back(this->Post([&] {
        return f(element);
      }));
    }
    for (auto& future : futures) {
      if (!future.valid()) {
        LOG_FATAL << "Unknow reason: future is invalid!";
      }
      *(out++) = future.get();
    }
    return out;
  }

  /**
   * @brief 将函数应用于输入元素的范围而不存储结果
   *
   */
  template <typename InputIter, typename F>
  void ForEach(InputIter begin, InputIter end, F&& f) {
    typedef std::future<void> Future;
    std::vector<Future> futures;
    for (auto iter = begin; iter != end; ++iter) {
      auto& element = *iter;
      // we need wait for result, so & is fine here
      futures.emplace_back(this->Post([&] {
        f(element);
      }));
    }
    for (auto& future : futures) {
      if (!future.valid()) {
        LOG_FATAL << "Unknow reason: future is invalid!";
      }
      future.get();
    }
  }

  /**
   * @brief 将任务提交到线程池中并返回一个 std::future 对象
   *
   */
  template <class FuncType, class... Args>
  auto SafePost(FuncType f, Args... args) -> std::future<decltype(f(args...))> {
    return Post(f, args...);
  }

 private:
  /**
   * @brief 将任务提交到线程池中并返回一个 std::future 对象
   *
   * @note  和 this->SafePost 方法不同的是, Post 参数使用了万能引用而 SafePost 参数是按值传递. 这意味着 Post
   * 函数模板可以接收更多类型的可调用对象和参数类型, 使用起来也更加复杂; SafePost 函数模板更加简单易用,
   * 但是只能接收一些特定类型的可调用对象和参数类型
   *
   *        1. Post 方法的 FuncType 参数可以接受任意类型的可调用对象(包括函数指针, 函数对象, 成员函数指针和
   * lambda表达式等), SafePost 方法的 FuncType 参数只能接受可调用对象的值类型(函数指针或者函数对象).
   *        2. Post 方法的 Args 参数可以接收任意类型的参数, SafePost 方法的的 Args 参数只能接收参数的值类型
   */
  template <class FuncType, class... Args>
  auto Post(FuncType&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    CHECK(is_running_) << "ThreadPool [" << pool_name_ << "_" << pool_index_ << "_" << pool_level_
                       << "] has been stopped";

    using ReturnType = decltype(f(args...));
    auto pack_task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<FuncType>(f), std::forward<Args>(args)...));
    std::future<ReturnType> ret_future = pack_task->get_future();

    {
      std::lock_guard<std::mutex> lock(task_lock_);
      tasks_.emplace([pack_task]() {
        (*pack_task)();
      });
      task_cv_.notify_one();
    }
    return ret_future;
  }

 private:
  // 线程池
  std::vector<std::thread> pool_;
  std::string pool_name_;
  int32_t pool_index_ = -1;
  int32_t pool_level_ = -1;

  // 任务队列
  std::queue<std::function<void()>> tasks_;
  std::mutex task_lock_;
  std::condition_variable task_cv_;

  std::atomic<bool> is_running_ = {false};
  std::atomic<uint32_t> idl_thread_num_ = {0};
  uint32_t size_ = {0};
  std::atomic<bool> enable_check_and_log_ = {false};
};

}  // namespace util
}  // namespace auto_pilot

#include <string>

#include "logger/logger.h"
#include "util/thread_pool/thread_pool.h"
#include "util/thread_pool/thread_pool_util.h"
#include "util/time/time.h"

namespace auto_pilot {
namespace util {

ThreadPool::ThreadPool(const std::string& name, const uint32_t number, const uint32_t thread_pool_index,
                       const uint32_t thread_pool_level)
    : pool_name_(name), pool_index_(thread_pool_index), pool_level_(thread_pool_level), size_(number) {
  Start();
}

void ThreadPool::Start() {
  // 避免重复初始化
  if (is_running_.exchange(true)) {
    return;
  }

  for (uint32_t i = 0; i < size_; ++i) {
    pool_.emplace_back([this] {
      ThreadPoolUtil::set_s_thread_pool_index(pool_index_);
      ThreadPoolUtil::set_s_thread_pool_next_level(pool_level_ + 1);

      // 线程池中的线程名格式为: ${pool_name}_${pool_index}_${pool_level}
      std::string thread_name = pool_name_ + "_" + std::to_string(pool_index_) + "_" + std::to_string(pool_level_);
      pthread_setname_np(pthread_self(), thread_name.c_str());
      while (is_running_) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(task_lock_);
          if (!is_running_ && tasks_.empty()) {
            return;
          }
          task_cv_.wait(lock, [this] {
            return !is_running_ || !tasks_.empty();
          });
          if (!is_running_ && tasks_.empty()) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
        }
        --idl_thread_num_;
        time::Time start_time = time::Time::Now();
        task();
        if (enable_check_and_log_) {
          constexpr int64_t max_interval_ns = 500 * 1000 * 1000;  // 500ms
          time::Time end_time = time::Time::Now();
          time::Duration duration = end_time - start_time;
          if (duration.ToNanosecond() > max_interval_ns) {
            LOG_WARN << "\nThread task takes " << duration.ToNanosecond() << "ns more than 500ms"
                     << "; start: " << start_time.ToString() << "; end: " << end_time.ToString();
          }
        }
        ++idl_thread_num_;
      }
    });
    ++idl_thread_num_;
  }
  LOG_INFO << "Thread pool started! index: " << pool_index_ << ", level: " << pool_level_
           << ", thread number: " << size_;
  ThreadPoolUtil::instance()->RecordRunningThreadPool(pool_index_, pool_level_);
}

ThreadPool::~ThreadPool() {
  Stop();
}

void ThreadPool::Stop() {
  // 避免重复销毁
  if (!is_running_.exchange(false)) {
    return;
  }

  {
    std::unique_lock<std::mutex> lock(task_lock_);
    is_running_ = false;
    task_cv_.notify_all();
  }

  for (auto& thr : pool_) {
    if (thr.joinable()) {
      thr.join();
    }
  }

  LOG_INFO << "Thread pool stop success! index: " << pool_index_ << ", level: " << pool_level_;
}

void ThreadPool::Wait() {
  while (!tasks_.empty() || idl_thread_num() != size()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

uint32_t ThreadPool::idl_thread_num() const {
  return idl_thread_num_;
}

uint32_t ThreadPool::size() const {
  return size_;
}

void ThreadPool::EnableCheck() {
  enable_check_and_log_.exchange(true);
}

void ThreadPool::DisableCheck() {
  enable_check_and_log_.exchange(false);
}

}  // namespace util
}  // namespace auto_pilot
