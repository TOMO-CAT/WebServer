
#include "logger/sync_file_appender.h"

#include "logger/log_backtrace.h"

namespace logger {

bool SyncFileAppender::Init() {
  return true;
}

void SyncFileAppender::Shutdown() {
}

void SyncFileAppender::Write(const std::shared_ptr<LogMessage>& log_message) {
  this->DumpToDisk(log_message->ToString());
  if (log_message->level() == Level::FATAL_LEVEL) {
    ::exit(1);
  }
}

}  // namespace logger
