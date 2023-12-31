#include "logger/file_appender.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "util/macros/io.h"

namespace logger {

FileAppender::FileAppender(std::string dir, std::string file_name, int retain_hours, bool is_cut)
    : file_dir_(dir), file_name_(file_name), retain_hours_(retain_hours), is_cut_(is_cut) {
  if (file_dir_.empty()) {
    file_dir_ = ".";
  }
  file_path_ = file_dir_ + "/" + file_name_ + "." + std::to_string(::getpid());
  last_hour_suffix_ = GenNowHourSuffix();
  pthread_mutex_init(&write_mutex_, nullptr);
}

FileAppender::~FileAppender() {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
}

bool FileAppender::OpenFile() {
  // 检查日志目录是否存在, 不存在则创建
  int ret = mkdir(file_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (ret != 0 && errno != EEXIST) {
    PRINT_TO_CONSOLE("mkdir fail, dir:%s err:%s", file_dir_.c_str(), strerror(errno));
    return false;
  }
  file_stream_.open(file_path_.c_str(), std::fstream::out | std::fstream::app);
  return true;
}

void FileAppender::DumpToDisk(const std::string& log_content) {
  // 将创建文件的时机延迟到第一次写日志的时候
  if (!is_receive_first_log) {
    this->OpenFile();
    is_receive_first_log = true;
  }

  CutIfNeed();
  pthread_mutex_lock(&write_mutex_);
  if (file_stream_.is_open()) {
    file_stream_ << log_content << "\n";
    file_stream_.flush();
  }
  pthread_mutex_unlock(&write_mutex_);
}

/**
 * 生成当前小时的文件 suffix, 格式 YYYYMMDDhh
 * eg: 2022040214
 */
int64_t FileAppender::GenNowHourSuffix() {
  struct timeval now;
  ::gettimeofday(&now, nullptr);
  return GenHourSuffix(&now);
}

int64_t FileAppender::GenHourSuffix(const struct timeval* tv) {
  struct tm tm_val;
  ::localtime_r(&tv->tv_sec, &tm_val);
  return static_cast<int64_t>(tm_val.tm_hour) + static_cast<int64_t>(tm_val.tm_mday) * 100 +
         static_cast<int64_t>(tm_val.tm_mon + 1) * 10000 + static_cast<int64_t>(tm_val.tm_year + 1900) * 1000000;
  // 每分钟切割一次
  // return (static_cast<int64_t>(tm_val.tm_hour) + static_cast<int64_t>(tm_val.tm_mday) * 100 +
  //         static_cast<int64_t>(tm_val.tm_mon + 1) * 10000 + static_cast<int64_t>(tm_val.tm_year +
  //         1900) * 1000000) *
  //            100 +
  //        tm_val.tm_min;
}

void FileAppender::CutIfNeed() {
  if (!is_cut_) {
    return;
  }

  struct timeval now;
  ::gettimeofday(&now, nullptr);

  int64_t now_hour_suffix = GenHourSuffix(&now);
  if (now_hour_suffix > last_hour_suffix_) {
    pthread_mutex_lock(&write_mutex_);
    if (now_hour_suffix > last_hour_suffix_) {
      std::string new_file_path = file_path_ + "." + std::to_string(last_hour_suffix_);  // eg: logger.log.YYYYMMDDhh
      int ret = rename(file_path_.c_str(), new_file_path.c_str());
      if (ret != 0) {
        PRINT_TO_CONSOLE("rename fail, old_file:%s new_file:%s err:%s", file_path_.c_str(), new_file_path.c_str(),
                         strerror(errno));
      }
      file_stream_.close();
      file_stream_.open(file_path_.c_str(), std::fstream::out | std::fstream::app);
#ifndef NDEBUG
      PRINT_TO_CONSOLE("cut file, last hour:%ld now hour:%ld file_path:%s new_file_path:%s", last_hour_suffix_,
                       now_hour_suffix, file_path_.c_str(), new_file_path.c_str());
#endif
      // 只有需要删除历史日志时才记录历史文件
      if (retain_hours_ > 0) {
#ifndef NDEBUG
        PRINT_TO_CONSOLE("[debug] insert history file:%ld", last_hour_suffix_);
#endif
        history_files_.insert(last_hour_suffix_);
        DeleteOverdueFile(now_hour_suffix);
      }
      last_hour_suffix_ = now_hour_suffix;
    }
    pthread_mutex_unlock(&write_mutex_);
  }
}

void FileAppender::DeleteOverdueFile(int64_t now_hour_suffix) {
  if (retain_hours_ <= 0) {
    return;
  }

  // 不能在 for 循环中 erase, 会使得迭代器失效
  // for (int64_t hour_suffix : history_files_) {
  //   if (now_hour_suffix >= hour_suffix + retain_hours_) {
  //     std::string old_file_path = file_path_ + "." + std::to_string(hour_suffix);
  //     ::remove(old_file_path.c_str());
  //     history_files_.erase(hour_suffix);
  //     PRINT_TO_CONSOLE("delete old file, file_path:%s", old_file_path.c_str());
  //   }
  // }

  // 在循环中使用 erase 删除 set 元素会导致迭代器失效, 从而可能导致未定义的行为。
  // 因此在循环中删除 set 元素时, 应该使用迭代器进行删除, 而不是使用循环变量或者其他迭代器。
  // 可以使用迭代器自身的后缀自增运算符 (iterator++) 来遍历集合, 并使用 iterator =
  // set.erase(iterator) 来删除元素。 这样可以确保迭代器指向下一个有效元素。
  for (auto it = history_files_.begin(); it != history_files_.end();) {
    int64_t hour_suffix = *it;
#ifndef NDEBUG
    PRINT_TO_CONSOLE("[debug] hour_suffix:%ld, now_hour_suffix:%ld, retain_hours=%d", hour_suffix, now_hour_suffix,
                     retain_hours_);
#endif
    if (now_hour_suffix > hour_suffix + retain_hours_) {
      std::string old_file_path = file_path_ + "." + std::to_string(hour_suffix);
      ::remove(old_file_path.c_str());
      it = history_files_.erase(it);
#ifndef NDEBUG
      PRINT_TO_CONSOLE("delete old file, file_path:%s", old_file_path.c_str());
#endif
    } else {
      ++it;
    }
  }
}

}  // namespace logger
