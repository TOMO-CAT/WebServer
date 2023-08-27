#pragma once

#include <string>

#include "logger/log_level.h"

namespace logger {

class LogMessage final {
 public:
  explicit LogMessage(const std::string& msg) : msg_(msg) {
  }
  ~LogMessage() = default;

 public:
  std::string ToString() const {
    return msg_;
  }

 private:
  std::string msg_;
};

}  // namespace logger
