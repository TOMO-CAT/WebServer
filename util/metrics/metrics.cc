#include "util/metrics/metrics.h"

#include <pthread.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>

#include "logger/log.h"
#include "prometheus/counter.h"
#include "prometheus/detail/utils.h"
#include "prometheus/exposer.h"
#include "prometheus/gauge.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"
#include "util/sync/thread_safe_queue.hpp"

namespace util {
namespace metrics {

Metrics::Metrics() {
}

Metrics& Metrics::Instance() {
  static Metrics instance;
  return instance;
}

Metrics::~Metrics() {
  this->is_running_ = false;
  if (this->bg_thread_.joinable()) {
    this->bg_thread_.join();
  }
}

void Metrics::Init(const std::string& prefix, const std::string& addr, float sample_rate) {
  this->prefix_ = prefix;
  this->exposer_ = std::make_shared<prometheus::Exposer>(addr);
  this->registry_ = std::make_shared<prometheus::Registry>();
  this->exposer_->RegisterCollectable(Metrics::Instance().registry_);
  this->queue_ = std::make_shared<util::sync::ThreadSafeQueue<EmitMsg>>(10000);
  this->is_running_ = true;
  this->sample_rate_ = sample_rate;
  this->bg_thread_ = std::thread([this]() {
    EmitMsg msg;
    while (this->is_running_) {
      bool ok = this->queue_->DequeueTimeout(10 * 1000 * 1000, &msg);  // wait 10 ms
      if (!ok) {
        continue;
      }
      switch (msg.type) {
        case EmitType::COUNTER:
          this->emit_counter_impl(msg.name, msg.labels, msg.value);
          break;
        case EmitType::STORE:
          this->emit_store_impl(msg.name, msg.labels, msg.value);
          break;
        case EmitType::TIMER:
          this->emit_timer_impl(msg.name, msg.labels, msg.value);
          break;
        default:
          break;
      }
    }
  });
  pthread_setname_np(this->bg_thread_.native_handle(), "METRICS");
}

void Metrics::EmitCounter(const std::string& name, const std::map<std::string, std::string>& labels, double value) {
  Metrics::Instance().emit_counter_impl(name, labels, value);
}

void Metrics::EmitStore(const std::string& name, const std::map<std::string, std::string>& labels, double value) {
  Metrics::Instance().emit_store_impl(name, labels, value);
}

void Metrics::EmitTimer(const std::string& name, const std::map<std::string, std::string>& labels, double value) {
  static thread_local std::default_random_engine e;
  static thread_local std::uniform_real_distribution<> dis(0, 1);

  if (dis(e) > Metrics::Instance().sample_rate_) {
    return;
  }

  if (!Metrics::Instance().queue_->TryEnqueue(EmitMsg{.type = EmitType::TIMER, .labels = labels, .value = value})) {
    Metrics::Instance().emit_counter_impl("metrics_overflow", {}, 1);
  }
}

void Metrics::emit_counter_impl(const std::string& name, const std::map<std::string, std::string>& labels,
                                double value) {
  if (name.empty()) {
    return;
  }

  static std::unordered_map<std::string, prometheus::Family<prometheus::Counter>*> family_map;
  static std::unordered_map<
      std::string, std::unordered_map<prometheus::Labels, prometheus::Counter*, prometheus::detail::LabelHasher>>
      counter_map;

  static thread_local std::unordered_map<
      std::string, std::unordered_map<prometheus::Labels, prometheus::Counter*, prometheus::detail::LabelHasher>>
      self_counter_map;

  {
    auto name_iter = self_counter_map.find(name);
    if (name_iter != self_counter_map.end()) {
      auto counter_iter = name_iter->second.find(labels);
      if (counter_iter != name_iter->second.end()) {
        counter_iter->second->Increment(value);
        return;
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(this->mtx_);
    auto& instance = Metrics::Instance();
    if (family_map.count(name) == 0) {
      std::string metrics_name = name[0] == ':' ? name.substr(1) : instance.prefix_ + name;
      auto& family = prometheus::BuildCounter().Name(metrics_name).Register(*instance.registry_);
      family_map[name] = &family;
    }
    auto& family = *(family_map[name]);
    auto& counter = family.Add(labels);
    counter_map[name][labels] = &counter;
    self_counter_map = counter_map;
  }

  emit_counter_impl(name, labels, value);
}

void Metrics::emit_store_impl(const std::string& name, const std::map<std::string, std::string>& labels, double value) {
  if (name.empty()) return;
  static std::unordered_map<std::string, prometheus::Family<prometheus::Gauge>*> family_map;
  static std::unordered_map<std::string,
                            std::unordered_map<prometheus::Labels, prometheus::Gauge*, prometheus::detail::LabelHasher>>
      gauge_map;
  static thread_local std::unordered_map<
      std::string, std::unordered_map<prometheus::Labels, prometheus::Gauge*, prometheus::detail::LabelHasher>>
      self_gauge_map;

  {
    auto name_iter = self_gauge_map.find(name);
    if (name_iter != self_gauge_map.end()) {
      auto gauge_iter = name_iter->second.find(labels);
      if (gauge_iter != name_iter->second.end()) {
        gauge_iter->second->Set(value);
        return;
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(this->mtx_);
    auto& instance = Metrics::Instance();
    if (family_map.count(name) == 0) {
      std::string metrics_name = name[0] == ':' ? name.substr(1) : instance.prefix_ + name;
      auto& family = prometheus::BuildGauge().Name(metrics_name).Register(*instance.registry_);
      family_map[name] = &family;
    }
    auto& family = *(family_map[name]);
    auto& counter = family.Add(labels);  // counter is owned by prometheus, so it is not a stack variable
    gauge_map[name][labels] = &counter;
    self_gauge_map = gauge_map;
  }

  emit_store_impl(name, labels, value);
}

void Metrics::emit_timer_impl(const std::string& name, const std::map<std::string, std::string>& labels, double value) {
  if (name.empty()) return;
  static std::unordered_map<std::string, prometheus::Family<prometheus::Summary>*> family_map;
  static std::unordered_map<
      std::string, std::unordered_map<prometheus::Labels, prometheus::Summary*, prometheus::detail::LabelHasher>>
      timer_map;
  static thread_local std::unordered_map<
      std::string, std::unordered_map<prometheus::Labels, prometheus::Summary*, prometheus::detail::LabelHasher>>
      self_timer_map;

  {
    auto name_iter = self_timer_map.find(name);
    if (name_iter != self_timer_map.end()) {
      auto counter_iter = name_iter->second.find(labels);
      if (counter_iter != name_iter->second.end()) {
        counter_iter->second->Observe(value);
      }
    }
  }
  {
    std::lock_guard<std::mutex> lock(this->mtx_);
    auto& instance = Metrics::Instance();
    if (family_map.count(name) == 0) {
      auto metrics_name = name[0] == ':' ? name.substr(1) : instance.prefix_ + name;
      auto& family = prometheus::BuildSummary().Name(metrics_name).Register(*instance.registry_);
      family_map[name] = &family;
    }
    auto& family = *(family_map[name]);
    auto& timer = family.Add(labels, prometheus::Summary::Quantiles{{0.5, 0.05}, {0.90, 0.01}, {0.99, 0.001}});
    timer_map[name][labels] = &timer;
    self_timer_map = timer_map;
  }
  emit_timer_impl(name, labels, value);
}

}  // namespace metrics
}  // namespace util
