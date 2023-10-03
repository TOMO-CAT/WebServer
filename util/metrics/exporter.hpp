#pragma once

#include <map>
#include <string>

#include "util/metrics/metrics.h"

namespace util {
namespace metrics {

inline void ExportLatency(std::string country, std::string component, std::string event, double latency_ms) {
  Metrics::Instance().EmitTimer("latency", {{"country", country}, {"component", component}, {"event", event}},
                                latency_ms);
}

inline void ExportCounter(std::string country, std::string component, std::string event, int val) {
  Metrics::Instance().EmitCounter("counter", {{"country", country}, {"component", component}, {"event", event}}, val);
}

inline void ExportError(std::string country, std::string component, std::string event, int val = 1) {
  Metrics::Instance().EmitCounter("error", {{"country", country}, {"component", component}, {"event", event}}, val);
}

inline void ExportGauge(std::string country, std::string component, std::string event, int val) {
  Metrics::Instance().EmitStore("gauge", {{"country", country}, {"component", component}, {"event", event}}, val);
}

inline void ExportLatency(const std::map<std::string, std::string>& labels, double latency_ms) {
  Metrics::Instance().EmitTimer("latency", labels, latency_ms);
}

inline void ExportCounter(const std::map<std::string, std::string>& labels, int val) {
  Metrics::Instance().EmitCounter("counter", labels, val);
}

inline void ExportError(const std::map<std::string, std::string>& labels, int val = 1) {
  Metrics::Instance().EmitCounter("error", labels, val);
}

inline void ExportGauge(const std::map<std::string, std::string>& labels, int val) {
  Metrics::Instance().EmitCounter("gauge", labels, val);
}

}  // namespace metrics
}  // namespace util
