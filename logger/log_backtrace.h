#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "util/macros/class_design.h"

namespace logger {

std::string Backtrace(const uint32_t skip_frame_depth = 1);

/**
 * @brief 打印函数调用栈
 *
 */
class StackDumper {
 public:
  explicit StackDumper(uint32_t skip);
  ~StackDumper();

 public:
  bool Dump(std::vector<std::string>* const stack_frames);

 private:
  char* Demangle(const char* name);
  int Backtrace(const char* file, int line, const char* func, uint32_t* const count,
                std::vector<std::string>* const stacks);

 private:
  static void ErrorCallback(void* data, const char* msg, int errnum);
  static int BacktraceCallback(void* data, uintptr_t pc, const char* file, int line, const char* func);

 private:
  uint32_t skip_ = 0;
  std::string exec_path_;
  char* demangle_buff_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(StackDumper);
};

}  // namespace logger
