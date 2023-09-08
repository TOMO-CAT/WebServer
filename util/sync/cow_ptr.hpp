#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include "logger/log.h"

namespace util {
namespace sync {

template <typename T>
class cow_ptr {
 public:
  cow_ptr() = default;
  ~cow_ptr() = default;
  explicit cow_ptr(const std::shared_ptr<T>& sp) : sp_(sp) {
  }

 public:
  cow_ptr(const cow_ptr& rhs) = delete;
  cow_ptr& operator=(const cow_ptr& rhs) = delete;
  cow_ptr(cow_ptr&& rhs) = delete;

 public:
  /**
   * @brief 读接口直接返回 sp_ 的拷贝, 使得 sp_ 的引用计数加一
   *
   * @return std::shared_ptr<const T>
   */
  inline std::shared_ptr<const T> Load() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return sp_;
  }

  /**
   * @brief 重置数据
   *
   * @param new_data
   */
  void Reset(const T& new_data) {
    this->Update([&new_data](T* data) {
      *data = new_data;
    });
  }

  /**
   * @brief 更新数据, 传入一个 Lambda 用于更新数据, 通过互斥锁保证写独占
   *
   * @tparam F
   * @param func
   */
  template <typename F>
  void Update(F&& func) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (!sp_.unique()) {
      sp_.reset(new T(*sp_));
    }
    CHECK(sp_.unique());  // 保证独占
    std::forward<F>(func)(sp_.get());
  }

 private:
  mutable std::mutex mtx_;
  std::shared_ptr<T> sp_;
};

}  // namespace sync
}  // namespace util
