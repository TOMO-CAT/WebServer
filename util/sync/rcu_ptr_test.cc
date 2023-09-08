#include "util/sync/rcu_ptr.hpp"

#include <sys/time.h>

#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "logger/log.h"
#include "util/random.h"
#include "util/time/timestamp.hpp"

namespace {
constexpr int READ_THREAD_COUNT = 20;   // read 线程数
constexpr int UPDATE_THREAD_COUNT = 2;  // update 线程数
constexpr int RESET_THREAD_COUNT = 2;   // reset 线程数

constexpr int READ_QPS = 10000;  // read QPS
constexpr int UPDATE_QPS = 20;   // update QPS
constexpr int RESET_QPS = 1;     // reset QPS

uint64_t g_start_ns = util::time::TimestampNanoSec();
constexpr uint64_t RUN_DURATION_SECONDS = 10;

}  // namespace

TEST(RcpPtrTest, rcu_ptr_test) {
  util::sync::rcu_ptr<std::vector<int>> rp(std::make_shared<std::vector<int>>());

  std::vector<std::thread> threads;

  // 构造 read 线程
  for (int i = 0; i < READ_THREAD_COUNT; i++) {
    threads.push_back(std::thread([&]() {
      uint64_t t_start_ns;
      int cnt;
      while (true) {
        t_start_ns = util::time::TimestampNanoSec();
        cnt = rp.Load()->size();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / READ_QPS * READ_THREAD_COUNT));
        LOG_INFO_EVERY(READ_THREAD_COUNT * READ_QPS)
            << "[reader] cnt: " << cnt << " cost [" << util::time::TimestampNanoSec() - t_start_ns << "] ns";
        if (t_start_ns - g_start_ns > RUN_DURATION_SECONDS * 1000000000) {
          return;
        }
      }
    }));
  }

  // 构造 update 线程
  for (int i = 0; i < UPDATE_THREAD_COUNT; i++) {
    threads.push_back(std::thread([&]() {
      uint64_t t_start_ns;
      while (1) {
        t_start_ns = util::time::TimestampNanoSec();
        rp.Update([](std::vector<int>* write_buffer_data) {
          for (int i = 0; i < 10; i++) {
            write_buffer_data->push_back(util::RandInt(100000));
          }
        });
        LOG_INFO_EVERY(UPDATE_THREAD_COUNT * UPDATE_QPS)
            << "[update writer] cost [" << util::time::TimestampNanoSec() - t_start_ns << "] ns";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / UPDATE_QPS * UPDATE_THREAD_COUNT));
        if (t_start_ns - g_start_ns > RUN_DURATION_SECONDS * 1000000000) {
          return;
        }
      }
    }));
  }

  // 构造 reset 线程
  for (int i = 0; i < RESET_THREAD_COUNT; i++) {
    threads.push_back(std::thread([&]() {
      uint64_t t_start_ns;
      while (1) {
        std::shared_ptr<std::vector<int>> new_data_sp = std::make_shared<std::vector<int>>();
        new_data_sp->reserve(100);
        for (int i = 0; i < 300; i++) {
          new_data_sp->push_back(util::RandInt(10000));
        }
        t_start_ns = util::time::TimestampNanoSec();
        rp.Store(std::move(new_data_sp));
        LOG_INFO_EVERY(RESET_THREAD_COUNT * RESET_QPS)
            << "[reset writer] cost [" << util::time::TimestampNanoSec() - t_start_ns << "] ns";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / RESET_QPS * RESET_THREAD_COUNT));
        if (t_start_ns - g_start_ns > RUN_DURATION_SECONDS * 1000000000) {
          return;
        }
      }
    }));
  }

  for (auto&& thread : threads) {
    thread.join();
  }
}
