#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "logger/file_appender.h"

namespace logger {

/**
 * @brief 同步日志落盘
 *
 */
class SyncFileAppender final : public FileAppender {
 public:
  /**
   * @brief Construct a new File Appender object
   *
   * @param dir 日志保存路径
   * @param file_name 日志名
   * @param retain_hours 保存小时数
   * @param is_cut 是否切割日志
   */
  SyncFileAppender(std::string dir, std::string file_name, int retain_hours, bool is_cut)
      : FileAppender(dir, file_name, retain_hours, is_cut) {
  }
  virtual ~SyncFileAppender() {
  }

 public:
  bool Init() override;
  void Shutdown() override;
  void Write(const std::shared_ptr<LogMessage>& log_message) override;

 private:
  std::atomic<bool> receive_fatal_ = {false};
};

}  // namespace logger
