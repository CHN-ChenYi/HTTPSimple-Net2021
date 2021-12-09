#pragma once

#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
 public:
  bool push(T const &value) {
    std::unique_lock<std::mutex> lock(mutex_);
    q_.push(value);
    return true;
  }
  bool pop(T &value) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (q_.empty()) return false;
    value = q_.front();
    q_.pop();
    return true;
  }
  bool empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return q_.empty();
  }

 private:
  std::queue<T> q_;
  mutable std::mutex mutex_;
};
