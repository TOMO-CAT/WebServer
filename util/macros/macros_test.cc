#include "gtest/gtest.h"

#include "util/macros/io.h"

namespace util {
namespace macros {

TEST(TestMacros, io_test) {
  PRINT_TO_CONSOLE("int=%d || str=%s || double=%.2f", 101, "cat", 3.1415926);

  errno = 10;  // No child processes
  PERROR_TO_CONSOLE("something is wrong");
  EXPECT_EQ(1, 1);
}

}  // namespace macros
}  // namespace util
