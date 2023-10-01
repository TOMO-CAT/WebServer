#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

using ThreadPoolIndex = uint32_t;
using ThreadPoolLevel = uint32_t;
// using ThreadPoolIndentifier = std::pair<ThreadPoolIndex, ThreadPoolLevel>;

class ThreadPool {};

// Level 表示线程池的嵌套深度, 通过隔离不同 Level 的线程池来避免饥饿死锁
class LevelThreadPool : public ThreadPool {
 public:
  LevelThreadPool(const std::string& name, const ThreadPoolIndex index, const ThreadPoolLevel level = 0);
  ~LevelThreadPool() = default;

 private:
  const std::string name_;
  const ThreadPoolIndex index_ = 0;
  const ThreadPoolLevel level_ = 0;
};

// 单例模式
class ThreadPoolUtil {
 public:
  static constexpr uint32_t kMaxThreadPoolCount = 10;          // 最多支持线程池数
  static constexpr uint32_t kMaxThreadPoolLevel = 5;           // 每个线程池最大支持嵌套深度
  static constexpr uint32_t kMaxTotalThreads = 200;            // 最大支持的全局线程数
  static constexpr char kDefaultThreadPoolName[] = "DEFAULT";  // 默认线程池的名称

 public:
  ThreadPoolUtil() {
  }

 public:
  std::shared_ptr<ThreadPool> GetThreadPool(const std::string& name, const uint32_t level);
  std::shared_ptr<ThreadPool> GetThreadPool(const uint32_t index, const uint32_t level);
  void RegisterThreadPool(const std::string& name);

 private:
  // 储存全部线程池
  std::shared_ptr<LevelThreadPool> thread_pools_[kMaxThreadPoolCount][kMaxThreadPoolLevel];
};
