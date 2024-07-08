#include "net/timer.h"

#include "gtest/gtest.h"
#include "logger/log.h"
#include "util/time/timestamp.hpp"

namespace net {

TEST(TimerTest, run) {
  bool called = false;

  auto fn_callback = [&called]() {
    called = true;
    LOG_INFO << "callback function is called";
  };

  util::time::Timestamp now = util::time::TimestampNanoSec();
  Timer timer(fn_callback, now, 0);

  EXPECT_EQ(timer.expiration(), now);
  EXPECT_FALSE(timer.repeat());

  EXPECT_FALSE(called);
  timer.Run();
  EXPECT_TRUE(called);
}

TEST(TimerTest, restart) {
  bool called = false;
  auto fn_callback = [&called]() {
    called = true;
    LOG_INFO << "callback function is called";
  };

  double interval_seconds = 1.0;
  util::time::Timestamp now = util::time::TimestampNanoSec();
  Timer timer(fn_callback, now, interval_seconds);

  EXPECT_TRUE(timer.repeat());

  EXPECT_FALSE(called);
  timer.Run();
  EXPECT_TRUE(called);

  timer.Restart(now);
  util::time::Timestamp next_expiration = util::time::SecondsLater(now, interval_seconds);
  EXPECT_EQ(timer.expiration(), next_expiration);
}

}  // namespace net
