#pragma once

#include <atomic>
#include <condition_variable>
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
    cv_.notify_one();
  }

  bool TryEnqueue(const T& elem) {
    std::unique_lock<std::shared_mutex> lk(rw_lock_);
    if (capacity_ > 0 && queue_.size() >= capacity_) {
      return false;
    }
    queue_.emplace_back(elem);
    cv_.notify_one();
    return true;
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

  bool DequeueTimeout(uint64_t timeout_ns, T* const element) {
    if (element == nullptr) {
      return false;
    }

    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    if (queue_.empty()) {
      cv_.wait_for(lock, std::chrono::nanoseconds(timeout_ns), [this]() {
        return this->break_all_wait_ == true || !this->queue_.empty();
      });

      if (break_all_wait_) {
        return false;
      }

      if (queue_.empty()) {
        return false;
      }
    }

    *element = std::move(queue_.front());
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

  void BreakAllWait() {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    break_all_wait_ = true;
    cv_.notify_all();
  }

  void Clear() {
    std::unique_lock<std::shared_mutex> lk(rw_lock_);
    while (!queue_.empty()) {
      queue_.pop_front();
    }
  }

  std::size_t Size() const {
    std::shared_lock<std::shared_mutex> lk(rw_lock_);
    return queue_.size();
  }

 private:
  std::atomic<bool> break_all_wait_ = {false};
  std::list<T> queue_;
  mutable std::shared_mutex rw_lock_;
  std::condition_variable_any cv_;
  std::size_t capacity_;
};

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const std::size_t capacity) : capacity_(capacity) {
}

template <typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
  BreakAllWait();
}

}  // namespace sync
}  // namespace util
