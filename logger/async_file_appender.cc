#include "logger/async_file_appender.h"

#include <iostream>
#include <list>
#include <memory>

#include "logger/backtrace.h"
#include "logger/file_appender.h"
#include "logger/log_message.h"
#include "util/sync/thread_safe_queue.hpp"

namespace logger {

namespace {
constexpr uint64_t kIntervalNanoSeconds = 5000'000'000;  // 异步打印日志的时间间隔
}

AsyncFileAppender::AsyncFileAppender(std::string dir, std::string file_name, int retain_hours, bool is_cut)
    : FileAppender(dir, file_name, retain_hours, is_cut) {
  message_queue_ = std::make_unique<::util::sync::ThreadSafeQueue<std::shared_ptr<LogMessage>>>(0);

  std::cout << "[DEBUG] 构造异步日志" << std::endl;

  thread_ = std::thread([this]() {
    ::pthread_setname_np(::pthread_self(), "ASYNC_LOG_APPENDER");

    last_timestamp_ns_ = std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);

    std::list<std::shared_ptr<LogMessage>> log_msg_list;
    while (is_running_ || !message_queue_->Empty()) {
      // 每隔 1 秒捞出 message_queue_ 中的所有日志
      // 这里通过条件变量可以保证程序退出时直接唤醒打印残存的异步日志
      // 这意味着异步日志罗盘最小时间间隔是 1 秒
      {
        std::unique_lock<std::mutex> lk(cv_mtx_);
        cv_.wait_for(lk, std::chrono::seconds(1), [this]() {
          return static_cast<bool>(!this->is_running_);
        });
      }

      std::list<std::shared_ptr<LogMessage>> temp_list;
      if (message_queue_->ExtractAll(&temp_list) && !temp_list.empty()) {
        log_msg_list.insert(log_msg_list.end(), temp_list.begin(), temp_list.end());
      }

      uint64_t current_timestamp_ns = std::chrono::system_clock::now().time_since_epoch() / std::chrono::nanoseconds(1);
      uint64_t delta_timestamp_ns = -last_timestamp_ns_;
      if (delta_timestamp_ns >= kIntervalNanoSeconds || !is_running_) {
        last_timestamp_ns_ = current_timestamp_ns;

        for (const auto& log_msg : log_msg_list) {
          this->DumpToDisk(log_msg->ToString());

          if (log_msg->level() == Level::FATAL_LEVEL) {
            ::exit(1);
          }
        }

        log_msg_list.clear();
      }
    }
  });
}

bool AsyncFileAppender::Init() {
  std::cout << "[DEBUG] 异步日志初始化成功" << std::endl;
  return true;
}

void AsyncFileAppender::Shutdown() {
  std::cout << "[DEBUG] 异步日志退出" << std::endl;
  is_running_.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void AsyncFileAppender::Write(const std::shared_ptr<LogMessage>& log_message) {
  this->message_queue_->Enqueue(log_message);
  std::cout << "[DEBUG] 异步日志长度 ";
}

}  // namespace logger
