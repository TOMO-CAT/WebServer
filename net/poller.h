#pragma once

#include <map>
#include <vector>

#include "util/macros/macros.h"
#include "util/time/timestamp.hpp"

namespace net {

class Channel;
class EventLoop;

/**
 * @brief IO 多路复用的基类, 用于支持 poll 和 epoll 两种 IO multiplexing 机制
 *
 * @note
 *   1. Poller 是 EventLoop 的间接成员, 只供其 Owner EventLoop 在 IO 线程中调用, 因此无需加锁
 *   2. Poller 生命周期和 EventLoop 一样
 *   3. Poller 并不持有 Channel, Channel 在析构之前必须自己 unregister (EventLoop::RemoveChannel), 避免空悬指针
 */
class Poller {
 public:
  using ChannelList = std::vector<Channel*>;

  enum class PollerType {
    kPollPoller,
    kEpollPoller,
  };

 public:
  explicit Poller(EventLoop* loop);
  virtual ~Poller();

 public:
  virtual ::util::time::Timestamp Poll(int timeout_ms, ChannelList* active_channels) = 0;
  virtual void UpdateChannel(Channel* channel) = 0;
  virtual void RemoveChannel(Channel* channel) = 0;
  virtual PollerType GetPollType() = 0;

 public:
  bool HasChannel(Channel* channel) const;
  void AssertInLoopThread() const;

  //  public:
  //   static Poller* NewDefaultPoller(EventLoop* loop);

 private:
  EventLoop* owner_loop_ = nullptr;

 protected:
  using ChannelMap = std::map<int, Channel*>;
  ChannelMap channel_map_;

 private:
  // 禁止拷贝
  DISALLOW_COPY_AND_ASSIGN(Poller);
};

}  // namespace net
