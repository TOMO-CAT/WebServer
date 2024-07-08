#include "util/time/timestamp.hpp"

#include "gtest/gtest.h"

namespace util {
namespace time {

TEST(TimestampTest, to_string) {
  Timestamp now = TimestampNanoSec();
  std::cout << "Timestamp [" << now << "] to string [" << TimestampToString(now) << "]" << std::endl;
}

}  // namespace time
}  // namespace util
