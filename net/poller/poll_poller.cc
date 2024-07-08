#include "net/poller/poll_poller.h"

#include <poll.h>
#include <sys/poll.h>

#include <algorithm>

#include "logger/log.h"
#include "net/channel.h"

namespace net {

PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {
}

PollPoller::~PollPoller() = default;

::util::time::Timestamp PollPoller::Poll(int timeout_ms, ChannelList* active_channels) {
  int events_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(), timeout_ms);
  int saved_errno = errno;
  util::time::Timestamp now = util::time::TimestampNanoSec();
  if (events_num > 0) {
    LOG_INFO << events_num << " events happened";
    FillActiveChannels(events_num, active_channels);
  } else if (events_num == 0) {
    LOG_INFO << "nothing happened";
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG_ERROR << "PollPoller::poll() fail with error " << ::strerror(saved_errno);
    }
  }
  return now;
}

void PollPoller::FillActiveChannels(const int events_num, ChannelList* const active_channels) const {
  int local_event_num = events_num;
  for (auto fd_iter = poll_fds_.begin(); fd_iter != poll_fds_.end(); ++fd_iter) {
    if (fd_iter->revents > 0) {
      --local_event_num;
      auto channel_iter = channel_map_.find(fd_iter->fd);
      CHECK(channel_iter != channel_map_.end());
      Channel* channel = channel_iter->second;
      CHECK_EQ(channel->fd(), fd_iter->fd);
      channel->set_revents(fd_iter->revents);
      active_channels->push_back(channel);
    }
  }
}

void PollPoller::UpdateChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  LOG_INFO << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0) {
    // a new one, add to pollfds_
    CHECK_EQ(channel_map_.find(channel->fd()), channel_map_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());  // NOLINT
    pfd.revents = 0;
    poll_fds_.push_back(pfd);
    int idx = static_cast<int>(poll_fds_.size() - 1);
    channel->set_index(idx);
    channel_map_[pfd.fd] = channel;
  } else {
    // update existing one
    CHECK_NE(channel_map_.find(channel->fd()), channel_map_.end());
    CHECK_EQ(channel_map_[channel->fd()], channel);
    int idx = channel->index();
    CHECK(0 <= idx && idx < static_cast<int>(poll_fds_.size()));
    struct pollfd& pfd = poll_fds_[idx];
    // FIXME: 这里是特殊设计的?
    CHECK(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());  // NOLINT
    pfd.revents = 0;
    if (channel->IsNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -channel->fd() - 1;
    }
  }
}

void PollPoller::RemoveChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  LOG_INFO << "fd = " << channel->fd();
  CHECK_NE(channel_map_.find(channel->fd()), channel_map_.end());
  CHECK_EQ(channel_map_[channel->fd()], channel);
  CHECK(channel->IsNoneEvent());
  int idx = channel->index();
  CHECK(0 <= idx && idx < static_cast<int>(poll_fds_.size()));
  const struct pollfd& pfd = poll_fds_[idx];
  CHECK(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
  size_t n = channel_map_.erase(channel->fd());
  CHECK_EQ(n, 1);
  if (idx == static_cast<int>(poll_fds_.size() - 1)) {
    // 如果恰好是 poll_fds_ 尾元素, 通过 pop_back 加速
    poll_fds_.pop_back();
  } else {
    // 否则的话通过 swap 加速
    int channel_fd_at_end = poll_fds_.back().fd;
    std::iter_swap(poll_fds_.begin() + idx, poll_fds_.end() - 1);
    if (channel_fd_at_end < 0) {
      channel_fd_at_end = -channel_fd_at_end - 1;
    }
    channel_map_[channel_fd_at_end]->set_index(idx);
    poll_fds_.pop_back();
  }
}

Poller::PollerType PollPoller::GetPollType() {
  return Poller::PollerType::kPollPoller;
}

}  // namespace net
