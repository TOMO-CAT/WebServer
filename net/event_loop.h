#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "net/poller.h"
#include "util/macros/macros.h"
#include "util/time/timestamp.hpp"

namespace net {

class Channel;

/**
 * @brief 事件循环
 *
 * @note one loop per thread 意味着每个线程只能有一个 EventLoop 对象, 创建了 EventLoop 的线程就是 IO 线程
 */
class EventLoop final {
 public:
  EventLoop(const Poller::PollerType poller_type);
  ~EventLoop();

 public:
  using Functor = std::function<void()>;

 public:
  // 开始事件循环, 必须要在创建 EventLoop 的 IO 线程里被调用
  void Loop();
  // 退出循环, 需要通过 shared_ptr<EventLoop> 调用确保线程安全
  void Quit();

  // 判断当前线程是否在 EventLoop 所在的 IO 线程
  bool IsInLoopThread() const;

  // 判断是否持有某个 Channel
  bool HasChannel(const Channel* const channel);
  // 移除 Channel
  void RemoveChannel(const Channel* const channel);
  // 更新 Channel
  void UpdateChannel(Channel* const channel);

  // 断言当前线程就是 EventLoop 所在的 IO 线程
  void AssertInLoopThread() const;

  void RunInLoop(Functor cb);
  void QueueInLoop(Functor cb);

  size_t QueueSize() const;

 public:
  // Poll 返回数据的时间
  util::time::Timestamp poll_return_time() const;
  int64_t iteration() const;

 private:
  // 创建当前 EventLoop 的线程 ID (即 IO 线程), 但是可能被其他线程持有这个 EventLoop
  const std::thread::id thread_id_;
  // 是否处于 Loop 循环中
  bool looping_ = false;
  // 是否停止
  std::atomic<bool> quit_ = false;

  util::time::Timestamp poll_return_time_ = 0;
  int64_t iteration_ = 0;

  std::unique_ptr<Poller> poller_;
  std::vector<Channel> channel_list_;

 private:
  // 禁止拷贝
  DISALLOW_COPY_AND_ASSIGN(EventLoop);
};

}  // namespace net
