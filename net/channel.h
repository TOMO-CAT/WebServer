#pragma once

#include <poll.h>

#include <functional>
#include <memory>
#include <string>

#include "util/macros/macros.h"
#include "util/time/timestamp.hpp"

namespace net {

class EventLoop;

/**
 * @brief
 *
 * @note
 *   1. 每个 Channel 对象只属于一个 EventLoop 对象, 因此它只属于某个 IO 线程
 *   2. 每个 Channel 对象只负责一个 fd 的 IO 事件分发, 但它并不拥有这个 fd
 */
class Channel final {
 public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(util::time::Timestamp)>;

 public:
  enum EventType {
    kNoneEvent = 0,
    kReadEvent = POLLIN | POLLPRI,
    kWriteEvent = POLLOUT,
  };

 public:
  Channel(EventLoop* loop, int fd);
  ~Channel();

 public:
  // 令 Channel 持有调用 Tie() 方法对象的弱指针, 避免在 HandleEvent 时 obj 对象析构·
  void Tie(const std::shared_ptr<void>& obj);

  void HandleEvent(const ::util::time::Timestamp receive_time);
  void SetReadCallback(const ReadEventCallback& cb);
  void SetWriteCallback(const EventCallback& cb);
  void SetCloseCallback(const EventCallback& cb);
  void SetErrorCallback(const EventCallback& cb);

  EventLoop* OwnerLoop() const;
  void Remove();
  bool IsNoneEvent() const;

  void EnableReading();
  void DisableReading();
  void EnableWriting();
  void DisableWriting();
  void DisableAll();
  bool IsWriting() const;
  bool IsReading() const;

 public:
  int fd() const;
  int index() const;
  int events() const;
  void set_index(const int index);
  void set_revents(const int revents);

  std::string ReventsToString() const;
  std::string EventsToString() const;

 private:
  void Update();
  void HandleEventWithGuard(const ::util::time::Timestamp receive_time);

 private:
  static std::string EventsToString(int fd, int events);

 private:
  std::weak_ptr<void> tie_;
  bool tied_ = false;            // 是否绑定了 tie_ 对象
  bool event_handling_ = false;  // 是否正在处理事件
  bool added_to_loop_ = false;

  EventLoop* loop_ = nullptr;
  const int fd_ = 0;
  int events_ = 0;
  int revents_ = 0;  // 来自 Poll / Epoll 的 receive events
  int index_ = -1;   // 象征着在 Poller 中的 poll_fds_ 索引

  ReadEventCallback read_callback_ = nullptr;
  EventCallback write_callback_ = nullptr;
  EventCallback close_callback_ = nullptr;
  EventCallback error_callback_ = nullptr;

 private:
  // 禁止拷贝
  DISALLOW_COPY_AND_ASSIGN(Channel);
};

}  // namespace net
