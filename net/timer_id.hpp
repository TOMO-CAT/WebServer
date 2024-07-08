#pragma once

#include "net/timer.h"
#include "util/macros/macros.h"

namespace net {

class Timer;

class TimerId {
 public:
  TimerId() {
  }
  TimerId(Timer* timer, int64_t seq) : timer_(timer), sequence_(seq) {
  }

  friend class TimerQueue;

 private:
  Timer* timer_ = nullptr;
  int64_t sequence_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TimerId);
};

}  // namespace net
