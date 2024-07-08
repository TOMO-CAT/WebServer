#include "net/poller/epoll_poller.h"

#include <sys/epoll.h>
#include <unistd.h>

#include "logger/log.h"
#include "net/channel.h"

namespace net {

namespace {
constexpr int32_t kNew = 1;
constexpr int32_t kAdded = 1;
constexpr int32_t kDeleted = 2;

}  // namespace

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop), epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {
  CHECK_GE(epoll_fd_, 0) << "epoll_create1 fail with error [" << strerror(errno) << "]";
}

EpollPoller::~EpollPoller() {
  ::close(epoll_fd_);
}

::util::time::Timestamp EpollPoller::Poll(int timeout_ms, ChannelList* active_channels) {
  LOG_INFO << "fd total count " << channel_map_.size();
  int events_num = ::epoll_wait(epoll_fd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);
  int saved_errno = errno;
  util::time::Timestamp now = util::time::TimestampNanoSec();
  if (events_num > 0) {
    LOG_INFO << events_num << " events happened";
    FillActiveChannels(events_num, active_channels);
    if (static_cast<size_t>(events_num) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (events_num == 0) {
    LOG_INFO << "nothing happened";
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG_ERROR << "EPollPoller::poll() fail with error " << ::strerror(saved_errno);
    }
  }
  return now;
}

void EpollPoller::FillActiveChannels(const int events_num, ChannelList* const active_channels) const {
  CHECK(events_num <= static_cast<int>(events_.size()));
  for (int i = 0; i < events_num; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    auto iter = channel_map_.find(fd);
    CHECK(iter != channel_map_.end());
    CHECK(iter->second == channel);
#endif
    channel->set_revents(events_[i].events);
    active_channels->push_back(channel);
  }
}

void EpollPoller::UpdateChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  const int index = channel->index();
  LOG_INFO << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;
  if (index == kNew || index == kDeleted) {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew) {
      CHECK(channel_map_.find(fd) == channel_map_.end());
      channel_map_[fd] = channel;
    } else {  // index == kDeleted
      CHECK(channel_map_.find(fd) != channel_map_.end());
      CHECK(channel_map_[fd] == channel);
    }
    channel->set_index(kAdded);
    Update(EPOLL_CTL_ADD, channel);
  } else {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    CHECK(channel_map_.find(fd) != channel_map_.end());
    CHECK(channel_map_[fd] == channel);
    CHECK(index == kAdded);
    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::RemoveChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  int fd = channel->fd();
  LOG_INFO << "fd = " << fd;
  CHECK(channel_map_.find(fd) != channel_map_.end());
  CHECK(channel_map_[fd] == channel);
  CHECK(channel->IsNoneEvent());
  int index = channel->index();
  CHECK(index == kAdded || index == kDeleted);
  size_t n = channel_map_.erase(fd);
  CHECK_EQ(n, 1);
  if (index == kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EpollPoller::Update(int operation, Channel* channel) {
  struct epoll_event event;
  ::memset(&event, 0, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_INFO << "epoll_ctl op = " << OperationToString(operation) << " fd = " << fd << " event = { "
           << channel->EventsToString() << " }";
  if (::epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_INFO << "epoll_ctl op =" << OperationToString(operation) << " fd =" << fd;
    } else {
      LOG_INFO << "epoll_ctl op =" << OperationToString(operation) << " fd =" << fd;
    }
  }
}

std::string EpollPoller::OperationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      CHECK(false) << "unsupported operation: [" << op << "]";
      return "Unknown Operation";
  }
}

}  // namespace net
