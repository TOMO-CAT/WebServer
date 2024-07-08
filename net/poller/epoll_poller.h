#pragma once

#include <string>
#include <vector>

#include "net/poller.h"

struct epoll_event;

namespace net {

class EpollPoller : public Poller {
 public:
  explicit EpollPoller(EventLoop* loop);
  ~EpollPoller();

 public:
  ::util::time::Timestamp Poll(int timeout_ms, ChannelList* active_channels) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;
  PollerType GetPollType() override;

 private:
  void FillActiveChannels(const int events_num, ChannelList* const) const;
  void Update(int operation, Channel* channel);

 private:
  int epoll_fd_ = -1;
  std::vector<struct epoll_event> events_;

 private:
  static constexpr int kInitEventListSize = 16;

 private:
  static std::string OperationToString(int op);
};

}  // namespace net
