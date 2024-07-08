#include "net/channel.h"

#include <sys/poll.h>

#include <memory>

#include "logger/log.h"
#include "net/event_loop.h"

namespace net {

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), index_(-1) {
}

Channel::~Channel() {
  CHECK(!event_handling_);
  CHECK(!added_to_loop_);
  if (loop_->IsInLoopThread()) {
    CHECK(!loop_->HasChannel(this));
  }
}

int Channel::fd() const {
  return fd_;
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::HandleEvent(const ::util::time::Timestamp receive_time) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(receive_time);
    } else {
      LOG_WARN << "HandleEvent fail because of expired tie object";
    }
  } else {
    HandleEventWithGuard(receive_time);
  }
}

/**
 * @brief 根据 Poller / Epoll 的不同事件调用对应的 callback 函数
 *
 * @param receive_time
 */
void Channel::HandleEventWithGuard(const ::util::time::Timestamp receive_time) {
  event_handling_ = true;
  LOG_INFO << ReventsToString();
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    // 表示挂断 (hangup) 或者连接中断
    LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    if (close_callback_) {
      close_callback_();
    }
  }
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_callback_) {
      error_callback_();
    }
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_callback_) {
      read_callback_(receive_time);
    }
  }
  if (revents_ & POLLOUT) {
    if (write_callback_) {
      write_callback_();
    }
  }
  event_handling_ = false;
}

int Channel::index() const {
  return index_;
}

void Channel::set_index(const int index) {
  index_ = index;
}

int Channel::events() const {
  return events_;
}

void Channel::set_revents(const int revents) {
  revents_ = revents;
}

EventLoop* Channel::OwnerLoop() const {
  return loop_;
}

void Channel::Remove() {
  CHECK(IsNoneEvent());
  added_to_loop_ = false;
  loop_->RemoveChannel(this);
}

bool Channel::IsNoneEvent() const {
  return events_ == EventType::kNoneEvent;
}

void Channel::Update() {
  added_to_loop_ = true;
  loop_->UpdateChannel(this);
}

void Channel::Remove() {
  CHECK(IsNoneEvent());
  added_to_loop_ = false;
  loop_->RemoveChannel(this);
}

std::string Channel::ReventsToString() const {
  return EventsToString(fd_, revents_);
}

std::string Channel::EventsToString() const {
  return EventsToString(fd_, events_);
}

std::string Channel::EventsToString(int fd, int events) {
  std::ostringstream oss;
  oss << fd << ": ";
  if (events & POLLIN) {
    oss << "IN ";
  }
  if (events & POLLPRI) {
    oss << "PRI ";
  }
  if (events & POLLOUT) {
    oss << "OUT ";
  }
  if (events & POLLHUP) {
    oss << "HUP ";
  }
  if (events & POLLRDHUP) {
    oss << "RDHUP ";
  }
  if (events & POLLERR) {
    oss << "ERR ";
  }
  if (events & POLLNVAL) {
    oss << "NVAL ";
  }

  return oss.str();
}

void Channel::EnableReading() {
  events_ |= kReadEvent;
  Update();
}

void Channel::DisableReading() {
  events_ &= ~kReadEvent;
  Update();
}

void Channel::EnableWriting() {
  events_ |= kWriteEvent;
  Update();
}

void Channel::DisableWriting() {
  events_ &= ~kWriteEvent;
  Update();
}

void Channel::DisableAll() {
  events_ = kNoneEvent;
  Update();
}

bool Channel::IsWriting() const {
  return events_ & kWriteEvent;
}

bool Channel::IsReading() const {
  return events_ & kReadEvent;
}

}  // namespace net
