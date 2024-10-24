#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "net/callbacks.h"
#include "net/channel.h"
#include "util/macros/macros.h"
#include "util/time/timestamp.hpp"

namespace net {

class EventLoop;
class Timer;
class TimerId;

/**
 * @brief 基于 timerfd 实现的定时器, 这样我们可以用和处理 IO 事件一样的逻辑来处理定时
 *
 * @note TimerQueue 只能在其所属的 IO 线程被调用, 因此不必加锁
 */
class TimerQueue {
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

 public:
  /**
   * @brief 注册定时任务
   * @note 在给定时间调用回调函数 cb, interval 大于 0 时会重复调用
   *
   * @param cb
   * @param when
   * @param interval_seconds
   * @return TimerId 注册的定时任务的唯一标识
   */
  TimerId AddTimer(const TimerCallback cb, util::time::Timestamp when, const double interval_seconds);
  /**
   * @brief 取消 timer_id 对应的定时任务
   *
   * @param timer_id
   */
  void Cancel(const TimerId& timer_id);

 private:
  // TODO: using std::unique_ptr<Timer> instead of raw pointers
  // 以 Entry 作为 Key 是因为如果仅仅用 Timestamp 作 key 可能处理两个 Timer 到期时间相同的情况
  using Entry = std::pair<util::time::Timestamp, Timer*>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<Timer*, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

 private:
  void AddTimerInLoop(Timer* timer);
  void CancelInLoop(const TimerId& timer_id);
  void HandleRead();
  // 获取并删除所有过期的 Timer
  std::vector<Entry> RemoveExpiredTimers(const util::time::Timestamp now);
  void Reset(const std::vector<Entry>& expired, const util::time::Timestamp now);
  bool Insert(Timer* const timer);

 private:
  EventLoop* loop_;
  const int timerfd_;
  // 使用 Channel 来观察 timerfd_ 上的 readable 事件
  Channel timerfd_channel_;
  // 按照 expiration 排序的 Timer 列表
  TimerList timers_;

  ///
  /// for cancel()
  ///
  ActiveTimerSet active_timers_;
  std::atomic<bool> calling_expired_timers_ = {false};
  ActiveTimerSet canceling_timers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TimerQueue);
};

}  // namespace net
