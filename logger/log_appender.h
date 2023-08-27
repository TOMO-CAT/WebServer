#pragma once

#include <cstdarg>
#include <cstdio>
#include <memory>

#include "logger/log_message.h"

namespace logger {

/**
 * @brief 日志落盘抽象接口
 *
 */

class LogAppender {
 public:
  LogAppender() = default;
  virtual ~LogAppender() = default;

 public:
  /**
   * @brief 必要的初始化
   *
   * @return true
   * @return false
   */
  virtual bool Init() = 0;

  /**
   * @brief 销毁前的操作
   *
   */
  virtual void Shutdown() = 0;

  /**
   * @brief 将日志写入文件
   *
   * @param log_msg
   */
  virtual void Write(const std::shared_ptr<LogMessage>& log_msg) = 0;
};  // namespace logger

}  // namespace logger
