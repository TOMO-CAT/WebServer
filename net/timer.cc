#include "net/timer.h"

#include <utility>

#include "util/time/timestamp.hpp"

namespace net {

std::atomic<int64_t> Timer::num_created_ = {0};

Timer::Timer(const TimerCallback cb, const util::time::Timestamp when, const double interval_seconds)
    : callback_(std::move(cb)),
      expiration_(when),
      interval_seconds_(interval_seconds),
      repeat_(interval_seconds > 0.0),
      sequence_(num_created_.fetch_add(1) + 1) {
}

void Timer::Run() const {
  this->callback_();
}

util::time::Timestamp Timer::expiration() const {
  return expiration_;
}

bool Timer::repeat() const {
  return repeat_;
}

int64_t Timer::sequence() const {
  return sequence_;
}

void Timer::Restart(const util::time::Timestamp now) {
  if (repeat_) {
    expiration_ = util::time::SecondsLater(now, interval_seconds_);
  } else {
    expiration_ = 0;
  }
}

int64_t Timer::num_created() {
  return num_created_.load();
}

}  // namespace net
