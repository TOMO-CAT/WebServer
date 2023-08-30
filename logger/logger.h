#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "logger/file_appender.h"
#include "logger/log_appender.h"

namespace logger {

class Logger {
 public:
  /**
   * 初始化, 未初始化或初始化失败时日志会输出到控制台
   */
  bool Init(const std::string& conf_path);
  /**
   * 获取单例
   */
  static Logger* Instance() {
    return instance_;
  }
  /**
   * 根据日志级别打印日志
   */
  void Log(Level log_level, const char* fmt, ...);

 public:
  static void set_trace_id(const uint64_t trace_id = 0);
  static uint64_t trace_id();

 private:
  Logger();
  ~Logger();

 private:
  static std::string GenLogPrefix();

  //  private:
  //   void Backtrace(const uint32_t skip_frames = 1);

 private:
  static Logger* instance_;
  bool is_console_output_ = true;
  std::unique_ptr<LogAppender> log_appender_ = nullptr;
  Level priority_ = Level::DEBUG_LEVEL;
  std::atomic<bool> receive_fatal_ = {false};

 private:
  static constexpr uint32_t kBufferSize = 4096;
  static __thread char buffer_[kBufferSize];

 private:
  DISALLOW_COPY_AND_ASSIGN(Logger);
};

}  // namespace logger
