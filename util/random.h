#pragma once

#include <cstdlib>

namespace util {

/**
 * @brief 返回 [min, max] 内的随机数
 *
 */
inline int32_t RandInt(int32_t min, int32_t max) {
  static unsigned int seed = 123;
  return min + ::rand_r(&seed) % (max - min);
}

/**
 * @brief 返回 [0, max] 内的随机数
 *
 */
inline int32_t RandInt(int32_t max) {
  static unsigned int seed = 123;
  return ::rand_r(&seed) % max;
}

}  // namespace util
