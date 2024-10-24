#include "net/timer_queue.h"

#include <bits/types/struct_itimerspec.h>
#include <bits/types/struct_timespec.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <functional>

#include "logger/log.h"
#include "net/event_loop.h"
#include "net/timer.h"
#include "net/timer_id.hpp"
#include "util/time/timestamp.hpp"

namespace net {

namespace {

int CreateTimerFd() {
  int timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timer_fd < 0) {
    LOG_FATAL << "timerfd_create fail";
  }
  return timer_fd;
}

/**
 * @brief 获取 when 和 now 之前的差值, 返回格式是 struct timespec
 *
 * @param when
 * @return struct timespec
 */
struct timespec HowMuchTimeFromNow(const util::time::Timestamp when) {
  uint64_t when_microseconds = util::time::TimestampToMicroseconds(when);
  uint64_t now_microseconds = util::time::TimestampMicroSec();
  CHECK_LT(when_microseconds, now_microseconds);
  int64_t microseconds_diff = when_microseconds - now_microseconds;
  if (microseconds_diff < 100) {
    microseconds_diff = 100;
  }
  struct timespec ts = {};
  ts.tv_sec = static_cast<time_t>(microseconds_diff / util::time::kSeconds2Microseconds);
  ts.tv_nsec = static_cast<uint64_t>((microseconds_diff % util::time::kSeconds2Microseconds) * 1000);
  return ts;
}

void ReadTimerFd(const int timer_fd, const util::time::Timestamp now) {
  uint64_t how_many = 0;
  ssize_t n = ::read(timer_fd, &how_many, sizeof how_many);
  LOG_INFO << "TimerQueue::handleRead() [" << how_many << "] at " << util::time::TimestampToString(now);
  if (n != sizeof how_many) {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

void ResetTimerFd(const int timer_fd, const util::time::Timestamp expiration) {
  // wake up loop by timerfd_settime()
  struct itimerspec new_value = {};
  struct itimerspec old_value = {};
  ::memset(&new_value, 0, sizeof new_value);
  ::memset(&old_value, 0, sizeof old_value);
  new_value.it_value = HowMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timer_fd, 0, &new_value, &old_value);
  if (ret) {
    LOG_ERROR << "timerfd_settime()";
  }
}

}  // namespace

TimerQueue::TimerQueue(EventLoop* loop) : loop_(loop), timerfd_(CreateTimerFd()), timerfd_channel_(loop, timerfd_) {
  timerfd_channel_.SetReadCallback(std::bind(&TimerQueue::HandleRead, this));
  timerfd_channel_.EnableReading();
}

TimerQueue::~TimerQueue() {
  timerfd_channel_.DisableAll();
  timerfd_channel_.Remove();
  ::close(timerfd_);
  // don't remove channel, since we're in EventLoop::dtor();
  for (const Entry& timer : timers_) {
    delete timer.second;
  }
}

TimerId TimerQueue::AddTimer(const TimerCallback cb, util::time::Timestamp when, const double interval_seconds) {
  Timer* timer = new Timer(std::move(cb), when, interval_seconds);
  loop_->RunInLoop(std::bind(&TimerQueue::AddTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::Cancel(const TimerId& timer_id) {
  loop_->RunInLoop(std::bind(&TimerQueue::CancelInLoop, this, timer_id));
}

void TimerQueue::AddTimerInLoop(Timer* timer) {
  loop_->AssertInLoopThread();
  bool earliest_changed = Insert(timer);
  if (earliest_changed) {
    ResetTimerFd(timerfd_, timer->expiration());
  }
}

void TimerQueue::CancelInLoop(const TimerId& timer_id) {
  loop_->AssertInLoopThread();
  CHECK_EQ(timers_.size(), active_timers_.size());
  ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
  ActiveTimerSet::iterator it = active_timers_.find(timer);
  if (it != active_timers_.end()) {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    CHECK_EQ(n, 1);
    (void)n;
    delete it->first;  // TODO: no delete please
    active_timers_.erase(it);
  } else if (calling_expired_timers_) {
    canceling_timers_.insert(timer);
  }
  CHECK_EQ(timers_.size(), active_timers_.size());
}

void TimerQueue::HandleRead() {
  loop_->AssertInLoopThread();
  util::time::Timestamp now = util::time::TimestampNanoSec();
  ReadTimerFd(timerfd_, now);

  std::vector<Entry> expired = RemoveExpiredTimers(now);

  calling_expired_timers_ = true;
  canceling_timers_.clear();
  // safe to callback outside critical section
  for (const Entry& it : expired) {
    it.second->Run();
  }
  calling_expired_timers_ = false;

  Reset(expired, now);
}

/**
 * @brief 从 timers_ 中移除已到期的 Timer
 *
 * @param now
 * @return std::vector<TimerQueue::Entry>
 */
std::vector<TimerQueue::Entry> TimerQueue::RemoveExpiredTimers(const util::time::Timestamp now) {
  CHECK_EQ(timers_.size(), active_timers_.size());
  std::vector<Entry> expired;

  // 哨兵值 (sentry) 作用是让 lower_bound 返回第一个未到期的 Timer 的迭代器
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  CHECK(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));

  // 从 timers_ 中移除已到期的 Timer
  timers_.erase(timers_.begin(), end);

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = active_timers_.erase(timer);
    CHECK_EQ(n, 1);
    (void)n;
  }

  CHECK_EQ(timers_.size(), active_timers_.size());

  // 编译器会实施 RVO (Return Value Optimization) 优化, 不必关心性能问题
  // 必要时可以考虑像 EventLoop::active_channels_ 一样复用 vector
  return expired;
}

void TimerQueue::Reset(const std::vector<Entry>& expired, const util::time::Timestamp now) {
  util::time::Timestamp next_expire = 0;

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repeat() && canceling_timers_.find(timer) == canceling_timers_.end()) {
      it.second->Restart(now);
      Insert(it.second);
    } else {
      // FIXME move to a free list
      delete it.second;  // FIXME: no delete please
    }
  }

  if (!timers_.empty()) {
    next_expire = timers_.begin()->second->expiration();
  }

  if (next_expire > 0) {
    ResetTimerFd(timerfd_, next_expire);
  }
}

bool TimerQueue::Insert(Timer* const timer) {
  loop_->AssertInLoopThread();
  CHECK_EQ(timers_.size(), active_timers_.size());
  bool earliest_changed = false;
  util::time::Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }
  {
    std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
    CHECK(result.second);
    (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result = active_timers_.insert(ActiveTimer(timer, timer->sequence()));
    CHECK(result.second);
    (void)result;
  }

  CHECK_EQ(timers_.size(), active_timers_.size());
  return earliest_changed;
}

}  // namespace net
