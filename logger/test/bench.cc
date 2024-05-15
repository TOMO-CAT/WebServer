#include <filesystem>
#include <string>

#include "logger/log.h"
#include "util/time/timestamp.hpp"

// 仿造 muduo/base/tests/Logging_test.cc
void bench() {
  const bool kLongLog = false;
  const uint32_t kLogCount = 1000 * 1000;
  uint64_t t_start_ns = util::time::TimestampNanoSec();

  std::string empty = " ";
  std::string long_str(3000, 'X');
  long_str += " ";

  for (uint32_t i = 0; i < kLogCount; ++i) {
    LOG_INFO << "Hello 0123456789"
             << " abcdefghijklmnopqrstuvwxyz" << (kLongLog ? long_str : empty) << i;
  }

  double t_cost_seconds = static_cast<double>(util::time::TimestampNanoSec() - t_start_ns) / 1000.0 / 1000.0 / 1000.0;
  printf("[Logger Bench] %f seconds || %10.2f msg/s\n", t_cost_seconds, kLogCount / t_cost_seconds);
}

int main() {
  // 初始化异步日志
  std::string path = std::filesystem::path(__FILE__).parent_path().string();
  if (!logger::Logger::Instance().Init(path + "/conf/logger.conf")) {
    LogError("init logger fail, print to console");
  }

  // 性能测试
  bench();
}
