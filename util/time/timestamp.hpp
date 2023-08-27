#pragma once

#include <sys/time.h>

#include <cstdint>
#include <ctime>

namespace util {
namespace time {

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

}  // namespace time
}  // namespace util
