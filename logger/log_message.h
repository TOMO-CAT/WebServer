#pragma once

#include <string>

#include "logger/log_level.h"

namespace logger {

class LogMessage final {
 public:
  LogMessage(const Level level, const std::string& msg) : msg_(msg), level_(level) {
  }
  ~LogMessage() = default;

 public:
  std::string ToString() const {
    return msg_;
  }

  Level level() {
    return level_;
  }

 private:
  std::string msg_;
  Level level_ = Level::INFO_LEVEL;
};

}  // namespace logger
