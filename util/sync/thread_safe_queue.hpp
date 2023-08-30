#pragma once

#include <cstdint>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace util {
namespace sync {

/**
 * @brief 线程安全队列
 *
 * @tparam T
 */
template <typename T>
class ThreadSafeQueue {
 public:
  explicit ThreadSafeQueue(const std::size_t capacity);
  ~ThreadSafeQueue();

 public:
  void Enqueue(const T& elem) {
    std::unique_lock<std::shared_mutex> lk(rw_lock_);
    queue_.emplace_back(elem);
    while (capacity_ > 0 && queue_.size() > capacity_) {
      queue_.pop_front();
    }
  }
  bool Dequeue(T* const elem) {
    if (elem == nullptr) {
      return false;
    }
    std::unique_lock<std::shared_mutex> lk(rw_lock_);
    if (queue_.empty()) {
      return false;
    }
    *elem = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  bool Empty() const {
    std::shared_lock<std::shared_mutex> lk(rw_lock_);
    return queue_.empty();
  }
  bool ExtractAll(std::list<T>* const elems) {
    if (elems == nullptr) {
      return false;
    }
    std::unique_lock<std::shared_mutex> lk(rw_lock_);
    elems->clear();
    while (!queue_.empty()) {
      elems->emplace_back(queue_.front());
      queue_.pop_front();
    }
    return true;
  }
  void Clear();

 private:
  std::list<T> queue_;
  mutable std::shared_mutex rw_lock_;
  std::size_t capacity_;
};

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const std::size_t capacity) : capacity_(capacity) {
}

template <typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
}

}  // namespace sync
}  // namespace util
