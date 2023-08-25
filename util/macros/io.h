#pragma once

#include <sys/time.h>

#include <cstring>
#include <string>

/**
 * 将格式化字符串打印到标准输出
 */
#define PRINT_TO_CONSOLE(fmt, args...)                                                          \
  do {                                                                                          \
    struct timeval now;                                                                         \
    ::gettimeofday(&now, nullptr);                                                              \
    struct tm tm_now;                                                                           \
    ::localtime_r(&now.tv_sec, &tm_now);                                                        \
    printf("[%04d-%02d-%02d %02d:%02d:%02d.%06ld][%s:%d][%s] " fmt "\n", tm_now.tm_year + 1900, \
           tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,     \
           now.tv_usec, __FILE__, __LINE__, __FUNCTION__, ##args);                              \
    fflush(stdout);                                                                             \
  } while (0)

/**
 * 将 perror 打印到标准错误
 */
#define PERROR_TO_CONSOLE(str)                                                             \
  do {                                                                                     \
    struct timeval now;                                                                    \
    ::gettimeofday(&now, nullptr);                                                         \
    struct tm tm_now;                                                                      \
    ::localtime_r(&now.tv_sec, &tm_now);                                                   \
    char buff[200];                                                                        \
    snprintf(buff, sizeof(buff), "[%04d-%02d-%02d %02d:%02d:%02d.%06ld][%s:%d][%s] ",      \
             tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour,     \
             tm_now.tm_min, tm_now.tm_sec, now.tv_usec, __FILE__, __LINE__, __FUNCTION__); \
    std::string new_str = std::string(buff) + str;                                         \
    perror(new_str.c_str());                                                               \
  } while (0)
