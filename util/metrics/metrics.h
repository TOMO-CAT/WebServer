#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "util/sync/thread_safe_queue.hpp"
#include "util/threadpool.h"

namespace prometheus {
class Exposer;
class Registry;
}  // namespace prometheus

namespace util {
namespace metrics {

class Metrics {
 public:
  enum class EmitType { COUNTER = 0, TIMER = 1, STORE = 2 };

  struct EmitMsg {
    enum EmitType type;
    std::string name;
    std::map<std::string, std::string> labels;
    double value;
  };

 private:
  Metrics();
  ~Metrics();

 public:
  static Metrics& Instance();
  void Init(const std::string& prefix, const std::string& addr, float sample_rate);

 public:
  static void EmitCounter(const std::string& name, const std::map<std::string, std::string>& labels, double value);
  static void EmitTimer(const std::string& name, const std::map<std::string, std::string>& labels, double value);
  static void EmitStore(const std::string& name, const std::map<std::string, std::string>& labels, double value);

 private:
  void emit_counter_impl(const std::string& name, const std::map<std::string, std::string>& labels, double value);
  void emit_timer_impl(const std::string& name, const std::map<std::string, std::string>& labels, double value);
  void emit_store_impl(const std::string& name, const std::map<std::string, std::string>& labels, double value);

 private:
  std::shared_ptr<prometheus::Exposer> exposer_ = nullptr;
  std::shared_ptr<prometheus::Registry> registry_ = nullptr;
  std::string prefix_;
  std::shared_ptr<util::sync::ThreadSafeQueue<EmitMsg>> queue_ = nullptr;
  std::thread bg_thread_;
  bool is_running_ = false;
  float sample_rate_;
  std::mutex mtx_;
};

}  // namespace metrics
}  // namespace util
