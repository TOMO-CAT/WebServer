#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include "logger/file_appender.h"
#include "logger/log_message.h"
#include "util/sync/thread_safe_queue.hpp"

namespace logger {

/**
 * @brief 异步日志落盘
 *
 */
class AsyncFileAppender final : public FileAppender {
 public:
  /**
   * @brief Construct a new File Appender object
   *
   * @param dir 日志保存路径
   * @param file_name 日志名
   * @param retain_hours 保存小时数
   * @param is_cut 是否切割日志
   */
  AsyncFileAppender(std::string dir, std::string file_name, int retain_hours, bool is_cut);
  virtual ~AsyncFileAppender() {
    Shutdown();
  }

 public:
  bool Init() override;
  void Shutdown() override;
  void Write(const std::shared_ptr<LogMessage>& log_message) override;

 private:
  std::thread thread_;
  std::atomic<bool> is_running_ = true;
  std::unique_ptr<::util::sync::ThreadSafeQueue<std::shared_ptr<LogMessage>>> message_queue_ = nullptr;
  mutable std::mutex cv_mtx_;
  std::condition_variable cv_;

  uint64_t last_timestamp_ns_ = 0;  // 上一次落盘时间戳(单位 ns), 用于判断是否到了日志落盘的时间点
  std::atomic<bool> receive_fatal_ = {false};
};

}  // namespace logger
