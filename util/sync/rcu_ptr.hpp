#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace util {
namespace sync {

template <typename T>
class rcu_ptr {
 public:
  rcu_ptr() = default;
  ~rcu_ptr() = default;
  explicit rcu_ptr(const std::shared_ptr<const T>& sp) : sp_(sp) {
  }

 public:
  rcu_ptr(const rcu_ptr& rhs) = delete;
  rcu_ptr& operator=(const rcu_ptr& rhs) = delete;
  rcu_ptr(rcu_ptr&&) = delete;

 public:
  /**
   * @brief 以值方式返回 std::shared_ptr<const T>, 因此是线程安全的
   *
   */
  inline std::shared_ptr<const T> Load() const {
    return std::atomic_load_explicit(&sp_, std::memory_order_consume);
  }

  /**
   * @brief 接收 std::shared_ptr<const T> 左值参数, 通过替换智能指针的方式 reset 数据
   *
   */
  inline void Store(const std::shared_ptr<const T>& r) {
    std::atomic_store_explicit(&sp_, r, std::memory_order_release);
  }

  /**
   * @brief 接收 std::shared_ptr<const T> 右值参数, 通过替换智能指针的方式 reset 数据
   *
   */
  inline void Store(const std::shared_ptr<const T>&& r) {
    std::atomic_store_explicit(&sp_, std::move(r), std::memory_order_release);
  }

  /**
   * @brief 接收一个 Lambda 表达式用于 update 数据, 如果有并发写, 它会被一直调用直到数据被 update
   *
   * @tparam F 是一个接收 T* 的 Lambda 表达式, 其中 T* 是原始数据深拷贝对象的指针
   * @param func
   */
  template <typename F>
  void Update(F&& func) {
    std::shared_ptr<const T> sp_copy = std::atomic_load_explicit(&sp_, std::memory_order_consume);
    std::shared_ptr<T> sp_deep_copy;
    do {
      if (sp_copy) {
        sp_deep_copy = std::make_shared<T>(*sp_copy);
      }
      std::forward<F>(func)(sp_deep_copy.get());
    } while (!std::atomic_compare_exchange_strong_explicit(&sp_, &sp_copy,
                                                           std::shared_ptr<const T>(std::move(sp_deep_copy)),
                                                           std::memory_order_release, std::memory_order_consume));
  }

 private:
  std::shared_ptr<const T> sp_;
};

}  // namespace sync
}  // namespace util
