#include "logger/logger.h"

#include <execinfo.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <array>
#include <cstdarg>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cpptoml/cpptoml.h"
#include "logger/async_file_appender.h"
#include "logger/backtrace.h"
#include "logger/log.h"
#include "logger/log_appender.h"
#include "logger/log_level.h"
#include "logger/log_message.h"
#include "logger/sync_file_appender.h"
#include "util/toml/util.h"

namespace logger {

__thread char Logger::buffer_[Logger::kBufferSize];

namespace {

/**
 * @brief 生成 trace_id
 *
 * @note 将 uuid 解析成 uint64_t 数组并取第一个元素作为 trace_id
 * @return uint64_t
 */
uint64_t GenerateTraceId() {
  uuid_t uuid;
  uuid_generate(uuid);

  uint64_t* trace_id_list = reinterpret_cast<uint64_t*>(uuid);
  return trace_id_list[0];
}

const std::unordered_map<Level, std::string> kLevel2Description = {
    {Level::DEBUG_LEVEL, "[DEBUG]"}, {Level::INFO_LEVEL, "[INFO]"},   {Level::WARN_LEVEL, "[WARN]"},
    {Level::ERROR_LEVEL, "[ERROR]"}, {Level::FATAL_LEVEL, "[FATAL]"},
};

// constexpr uint32_t kSkipFrames = 3;

/**
 * @brief 注册信号处理函数
 *
 */
void HandleSignal() {
  auto handler = [](int signal) {
    printf("receive signal: %d\n", signal);
    Logger::Instance().Log(Level::FATAL_LEVEL, "Exiting due to receive signal: %d", signal);
    ::exit(1);
  };

  signal(SIGHUP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  // signal(SIGCHLD, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  signal(SIGBUS, handler);   // 10: Bus error (bad memory access)
  signal(SIGSEGV, handler);  // 11: Invalid memory reference
  signal(SIGABRT, handler);  // 6: Abort signal from abort(3)
  signal(SIGILL, handler);   // 4: Illegal Instruction
  signal(SIGFPE, handler);   // 8: Floating point exception
}

}  // namespace

thread_local int t_pid = ::getpid();
thread_local uint64_t t_trace_id = 0;

// Logger* Logger::instance_ = new Logger();

Logger::Logger() : is_console_output_(true) {
  HandleSignal();
}

bool Logger::Init(const std::string& conf_path) {
  // 解析配置
  std::shared_ptr<cpptoml::table> g;
  try {
    g = cpptoml::parse_file(conf_path);
  } catch (const cpptoml::parse_exception& e) {
    LogError("parse logger conf fail, path:%s err:%s", conf_path.c_str(), e.what());
    return false;
  }

  int level = 1;
  std::string dir;
  std::string file_name;
  int retain_hours;
  bool is_async = false;  // 默认是同步日志
  if (::util::toml::ParseTomlValue(g, "Level", &level)) {
    if (level >= static_cast<int>(Level::DEBUG_LEVEL) && level <= static_cast<int>(Level::ERROR_LEVEL)) {
      priority_ = Level(level);
    }
  }
  if (!::util::toml::ParseTomlValue(g, "Directory", &dir)) {
    dir = "./log";
  }
  if (!::util::toml::ParseTomlValue(g, "FileName", &file_name)) {
    LogWarn("parse FileName config fail, print to console");
    return false;
  }
  if (!::util::toml::ParseTomlValue(g, "RetainHours", &retain_hours)) {
    retain_hours = 0;  // retain_hours 为 0 时不会删除过期日志
  }
  if (!::util::toml::ParseTomlValue(g, "IsAsync", &is_async)) {
    is_async = false;
  }

  // 构造 log_appender_ 进行日志落盘, 支持同步日志和异步日志两种方式
  if (is_async) {
    log_appender_ = std::make_unique<AsyncFileAppender>(dir, file_name, retain_hours, is_async);
  } else {
    log_appender_ = std::make_unique<SyncFileAppender>(dir, file_name, retain_hours, true);
  }

  if (!log_appender_->Init()) {
    return false;
  }

  // 默认不打印到控制台, 只会打印 error 和 fatal 的日志
  is_console_output_ = false;

  return true;
}

void Logger::Log(Level log_level, const char* fmt, ...) {
  if (log_level < priority_) {
    return;
  }

  if (!is_running_) {
    return;
  }

  std::string new_fmt = GenLogPrefix() + kLevel2Description.at(log_level) + fmt;
  // FATAL 日志需要打印堆栈
  if (log_level == Level::FATAL_LEVEL) {
    // 防止打印多个 FATAL 日志, 第一次收到 FATAL 日志后就不再接受日志
    if (receive_fatal_.exchange(true)) {
      is_running_ = false;
      return;
    }

    new_fmt += "\n\tExiting due to FATAL log";
    new_fmt += "\n\tCall Stack:";
    new_fmt += "\n" + Backtrace();
  }

  va_list args;
  {
    va_start(args, fmt);
    // ERROR 及 FATAL 日志输出到控制台
    if (is_console_output_ || log_level >= Level::ERROR_LEVEL) {
// https://stackoverflow.com/questions/36120717/correcting-format-string-is-not-a-string-literal-warning
#if defined(__has_warning)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
      vprintf((new_fmt + "\n").c_str(), args);
#if defined(__has_warning)
#pragma clang diagnostic pop
#endif
    }
    va_end(args);
  }

  {
    va_start(args, fmt);
    if (log_appender_) {
      // https://stackoverflow.com/questions/36120717/correcting-format-string-is-not-a-string-literal-warning
#if defined(__has_warning)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
      ::vsnprintf(buffer_, sizeof(buffer_), new_fmt.c_str(), args);
#if defined(__has_warning)
#pragma clang diagnostic pop
#endif

      log_appender_->Write(std::make_shared<LogMessage>(log_level, buffer_));
    }
    va_end(args);
  }
}

std::string Logger::GenLogPrefix() {
  struct timeval now;
  ::gettimeofday(&now, nullptr);
  struct tm tm_now;
  ::localtime_r(&now.tv_sec, &tm_now);
  char time_str[100];
  snprintf(time_str, sizeof(time_str), "[%04d-%02d-%02d %02d:%02d:%02d.%06ld][%d:%lx]", tm_now.tm_year + 1900,
           tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, now.tv_usec, t_pid,
           t_trace_id);
  return time_str;
}

void Logger::set_trace_id(const uint64_t trace_id) {
  if (trace_id == 0) {
    t_trace_id = GenerateTraceId();
  } else {
    t_trace_id = trace_id;
  }
}

uint64_t Logger::trace_id() {
  return t_trace_id;
}

}  // namespace logger
