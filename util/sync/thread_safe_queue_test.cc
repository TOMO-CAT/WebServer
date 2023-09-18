#include "util/sync/thread_safe_queue.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

namespace util {
namespace sync {

TEST(ThreadSafeQueue, usage_test) {
  bool is_running = true;
  auto reader = [&is_running](ThreadSafeQueue<int32_t>* const queue) {
    std::list<int32_t> elem_list;

    while (is_running) {
      std::cout << "[reader] current queue size is " << queue->Size() << std::endl;
      if (!queue->ExtractAll(&elem_list) || elem_list.empty()) {
        std::cout << "[reader] queue is empty" << std::endl;
      } else {
        std::cout << "[reader] read " << elem_list.size() << " data from queue" << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  };
  auto writer = [&is_running](ThreadSafeQueue<int32_t>* const queue) {
    int32_t num = 0;
    while (is_running) {
      ++num;
      queue->Enqueue(num);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  };

  ThreadSafeQueue<int32_t> queue(0);
  std::thread reader_thread(reader, &queue);
  std::thread writer_thread1(writer, &queue);
  std::thread writer_thread2(writer, &queue);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  is_running = false;
  if (reader_thread.joinable()) {
    reader_thread.join();
  }
  if (writer_thread1.joinable()) {
    writer_thread1.join();
  }
  if (writer_thread2.joinable()) {
    writer_thread2.join();
  }

  EXPECT_EQ(1, 1);
}

}  // namespace sync
}  // namespace util
