#include "util/threadpool.h"

#include <algorithm>
#include <future>
#include <iterator>

#include "gtest/gtest.h"

namespace util {

TEST(ThreadPoolTest, post_test) {
  std::vector<int> expected = {1, 4, 9, 16, 25, 36, 49, 64, 81, 100};
  std::vector<int> actual = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ThreadPool threadpool("post_test", 5);
  std::vector<std::future<void>> futures;

  for (size_t i = 0; i < actual.size(); ++i) {
    auto f = threadpool.Post([&actual, i]() {
      actual[i] *= actual[i];
    });
    futures.emplace_back(std::move(f));
  }
  for (auto&& f : futures) {
    f.get();
  }

  EXPECT_EQ(expected, actual);
}

}  // namespace util
