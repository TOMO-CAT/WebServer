#pragma once

#include <atomic>

#include "net/callbacks.h"
#include "util/macros/macros.h"
#include "util/time/timestamp.hpp"

namespace net {

class Timer {
 public:
  Timer(const TimerCallback cb, const util::time::Timestamp when, const double interval_seconds);

 public:
  // 运行回调函数
  void Run() const;
  // 重启定时器
  void Restart(const util::time::Timestamp now);

 public:
  // 获取触发时间
  util::time::Timestamp expiration() const;
  // 是否周期性触发
  bool repeat() const;
  // 全局的定时器序号
  int64_t sequence() const;

 public:
  static int64_t num_created();

 private:
  const TimerCallback callback_;
  util::time::Timestamp expiration_ = 0;
  const double interval_seconds_;
  const bool repeat_;
  const int64_t sequence_;

 private:
  // 全局运行的 timer 数
  static std::atomic<int64_t> num_created_;

 private:
  // 禁止拷贝
  DISALLOW_COPY_AND_ASSIGN(Timer);
};

}  // namespace net
