#pragma once

using ThreadPoolIndex = uint32_t;
using ThreadPoolLevel = uint32_t;

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
