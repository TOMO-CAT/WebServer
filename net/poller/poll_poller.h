#pragma once

#include <vector>

#include "net/poller.h"

struct pollfd;

namespace net {
/**
 * @brief 基于 poll(2) 实现的 IO Multiplexing 类
 *
 */
class PollPoller : public Poller {
 public:
  explicit PollPoller(EventLoop* loop);
  virtual ~PollPoller();

 public:
  ::util::time::Timestamp Poll(int timeout_ms, ChannelList* active_channels) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;
  PollerType GetPollType() override;

 private:
  void FillActiveChannels(const int events_num, ChannelList* const) const;

 private:
  using PollFdList = std::vector<struct pollfd>;
  PollFdList poll_fds_;

 private:
  constexpr static Poller::PollerType poll_type_ = Poller::PollerType::kEpollPoller;
};

}  // namespace net
