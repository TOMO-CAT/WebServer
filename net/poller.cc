#include "net/poller.h"

#include "net/channel.h"
#include "net/event_loop.h"

namespace net {

Poller::Poller(EventLoop* loop) : owner_loop_(loop) {
}

Poller::~Poller() = default;

bool Poller::HasChannel(Channel* channel) const {
  AssertInLoopThread();
  const auto iter = channel_map_.find(channel->fd());
  return iter != channel_map_.end() && iter->second == channel;
}

void Poller::AssertInLoopThread() const {
  owner_loop_->AssertInLoopThread();
}

}  // namespace net
