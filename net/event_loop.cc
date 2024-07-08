#include "net/event_loop.h"

#include <memory>
#include <thread>

#include "logger/log.h"
#include "net/channel.h"
#include "net/poller/epoll_poller.h"
#include "net/poller/poll_poller.h"

namespace net {

namespace {
thread_local EventLoop* t_ThisThreadEventLoop = nullptr;
}

EventLoop::EventLoop(const Poller::PollerType poller_type) : thread_id_(std::this_thread::get_id()) {
  LOG_INFO << "create EventLoop [" << this << "] in thread [" << std::this_thread::get_id();
  if (t_ThisThreadEventLoop != nullptr) {
    // 如果当前线程已经有一个正在工作的 EventLoop, 直接抛出异常退出
    LOG_FATAL << "another EventLoop [" << t_ThisThreadEventLoop << "] already exists in this thread ["
              << "]";
  } else {
    t_ThisThreadEventLoop = this;
  }

  switch (poller_type) {
    case Poller::PollerType::kPollPoller: {
      poller_ = std::make_unique<PollPoller>(this);
    }
    case Poller::PollerType::kEpollPoller: {
      poller_ = std::make_unique<EpollPoller>(this);
    }
    default: {
      CHECK(false) << "unsupported poller type: [" << static_cast<int>(poller_type) << "]";
    }
  }
}

EventLoop::~EventLoop() {
  LOG_INFO << "EventLoop [" << this << "] of thread [" << thread_id_ << "] destructs in thread ["
           << std::this_thread::get_id() << "]";
  t_ThisThreadEventLoop = nullptr;
}

void EventLoop::Loop() {
  CHECK(!looping_);
  AssertInLoopThread();
  looping_ = true;
  quit_ = false;
  LOG_INFO << "EventLoop [" << this << "] start looping";

  while (!quit_) {
  }
}

void EventLoop::Quit() {
  quit_ = true;
}
bool EventLoop::IsInLoopThread() const {
  return thread_id_ == std::this_thread::get_id();
}

bool EventLoop::HasChannel(const Channel* const channel) {
  (void)channel;
  // CHECK()
  // FIXME:
  return true;
}

void EventLoop::UpdateChannel(Channel* const channel) {
  CHECK(channel->OwnerLoop() == this);
  AssertInLoopThread();
  poller_->UpdateChannel(channel);
}

void EventLoop::AssertInLoopThread() const {
  if (!IsInLoopThread()) {
    LOG_FATAL << "EventLoop [" << this << "] was created in [" << thread_id_ << "] but now is running in ["
              << std::this_thread::get_id() << "]";
  }
}

util::time::Timestamp EventLoop::poll_return_time() const {
  return poll_return_time_;
}

int64_t EventLoop::iteration() const {
  return iteration_;
}

}  // namespace net
