#pragma once

#include <inttypes.h>
#include <sys/time.h>

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

namespace util {
namespace time {

constexpr uint64_t kSeconds2NanoSeconds = 1'000'000'000;
constexpr uint64_t kMicroseconds2Nanoseconds = 1000;
constexpr uint64_t kSeconds2Microseconds = 1000'000;

using Timestamp = uint64_t;

/**
 * @brief 返回时间戳， 单位: 秒
 *
 * @return uint64_t
 */
inline uint64_t TimestampSec() {
  struct timeval tv;
  ::gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}

/**
 * @brief 返回时间戳， 单位: 微秒
 *
 * @return uint64_t
 */
inline uint64_t TimestampMicroSec() {
  struct timeval time;
  ::gettimeofday(&time, NULL);
  return time.tv_sec * 1000 * 1000 + time.tv_usec;
}

/**
 * @brief 返回时间戳， 单位: 纳秒
 *
 * @return uint64_t
 */
inline uint64_t TimestampNanoSec() {
  struct timespec time;
  ::clock_gettime(CLOCK_REALTIME, &time);
  return time.tv_sec * 1000 * 1000 * 1000 + time.tv_nsec;
}

/**
 * @brief 返回 now 加上 seconds 秒后的时间戳
 *
 * @param now
 * @param seconds
 * @return Timestamp
 */
inline Timestamp SecondsLater(const Timestamp now, const double seconds) {
  return now + seconds * kSeconds2NanoSeconds;
}

/**
 * @brief 将时间戳
 *
 * @param t
 * @return uint64_t
 */
inline uint64_t TimestampToMicroseconds(const Timestamp t) {
  return static_cast<uint64_t>(t / kMicroseconds2Nanoseconds);
}

inline std::string TimestampToString(const Timestamp t) {
  char buf[32] = {0};
  uint64_t seconds = t / kSeconds2NanoSeconds;
  uint64_t microseconds = t / kMicroseconds2Nanoseconds;
  ::snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

}  // namespace time
}  // namespace util
