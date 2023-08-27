#pragma once

#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "logger/log_appender.h"
#include "logger/log_message.h"
#include "util/macros/class_design.h"

namespace logger {

/**
 * @brief 将日志同步写入到磁盘文件中
 *
 */
class FileAppender final : public LogAppender {
 public:
  /**
   * @brief Construct a new File Appender object
   *
   * @param dir 日志保存路径
   * @param file_name 日志名
   * @param retain_hours 保存小时数
   * @param is_cut 是否切割日志
   */
  FileAppender(std::string dir, std::string file_name, int retain_hours, bool is_cut);
  virtual ~FileAppender();

 public:
  bool Init() override;
  void Shutdown() override;
  // void Write(const char* fmt, va_list args) override;
  void Write(const std::shared_ptr<LogMessage>& log_message) override;

 private:
  static int64_t GenNowHourSuffix();
  static int64_t GenHourSuffix(const struct timeval* tv);
  void CutIfNeed();
  void DeleteOverdueFile(int64_t now_hour_suffix);
  bool OpenFile();

 private:
  static constexpr uint32_t kFileAppenderBuffSize = 4096;

 private:
  std::fstream file_stream_;
  std::string file_dir_;
  std::string file_name_;
  std::string file_path_;
  int retain_hours_ = 0;
  int64_t last_hour_suffix_ = -1;
  pthread_mutex_t write_mutex_;
  bool is_cut_ = true;
  bool is_receive_first_log = false;  // 是否收到首条日志, 用于延迟创建日志文件
  std::set<int64_t> history_files_;

  DISALLOW_COPY_AND_ASSIGN(FileAppender);
};

}  // namespace logger
