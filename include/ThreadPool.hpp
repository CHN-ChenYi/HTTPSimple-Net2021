#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "ThreadSafeQueue.hpp"

#ifdef _DEBUG
#include <iostream>
#endif

class ThreadPool {
  std::vector<std::unique_ptr<std::thread>> threads_;
  std::vector<std::shared_ptr<std::atomic<bool>>> flags_;
  ThreadSafeQueue<std::function<void(int id)> *> q_;
  std::atomic<bool> is_done_;
  std::atomic<bool> is_stop_;
  std::atomic<int> waiting_num_;

  std::mutex mutex_;
  std::condition_variable cv_;

  void Init() {
    waiting_num_ = 0;
    is_stop_ = false;
    is_done_ = false;
  }

  /**
   * @brief set up a new thread
   *
   * @param i the index of the thread
   */
  void InitThread(int i) {
    std::shared_ptr<std::atomic<bool>> flag_ptr(flags_[i]);
    auto f = [this, i, flag_ptr]() {
      std::atomic<bool> &flag = *flag_ptr;
      std::function<void(int id)> *f_ptr;
      bool isPop = q_.pop(f_ptr);
      for (;;) {
        while (isPop) {
          // at return, delete the function
          std::unique_ptr<std::function<void(int id)>> func(f_ptr);
          (*f_ptr)(i);
          if (flag)
            return;
          else
            isPop = q_.pop(f_ptr);
        }
        // the queue is empty here, wait for the next command
        std::unique_lock<std::mutex> lock(mutex_);
        ++waiting_num_;
        cv_.wait(lock, [this, &f_ptr, &isPop, &flag]() {
          isPop = q_.pop(f_ptr);
          return isPop || is_done_ || flag;
        });
        --waiting_num_;
        if (!isPop) return;
      }
    };
    // threads[i].reset(new std::thread(f));
    threads_[i] = std::make_unique<decltype(std::thread(f))>(std::thread(f));
  }

  /**
   * @brief delete all the functions in the queue
   *
   */
  void ClearQueue() {
    std::function<void(int id)> *f;
    while (q_.pop(f)) delete f;
  }

 public:
  /**
   * @brief Construct a new Thread Pool object
   *
   */
  ThreadPool() {
    Init();
    resize(1);
  }

  /**
   * @brief Construct a new Thread Pool object
   *
   * @param thread_num the number of threads in the pool
   */
  ThreadPool(const int &thread_num) {
    Init();
    resize(thread_num);
  }

  /**
   * @brief Destroy the Thread Pool object
   *
   */
  ~ThreadPool() { Stop(); }

  /**
   * @brief get the number of threads in the pool
   *
   * @return the number of threads
   */
  int size() { return static_cast<int>(threads_.size()); }

  /**
   * @brief modify the number of threads in the pool
   *
   * @param thread_num the number of threads
   */
  void resize(const int &thread_num) {
    if (!is_stop_ && !is_done_) {
      int old_thread_num = static_cast<int>(threads_.size());
      if (old_thread_num <= thread_num) {
        threads_.resize(thread_num);
        flags_.resize(thread_num);
        for (int i = old_thread_num; i < thread_num; ++i) {
          flags_[i] = std::make_shared<std::atomic<bool>>(false);
          InitThread(i);
        }
      } else {
        for (int i = old_thread_num - 1; i >= thread_num; --i) {
          *flags_[i] = true;
          threads_[i]->detach();
        }
        {  // stop the detached threads that were waiting
          std::unique_lock<std::mutex> lock(mutex_);
          cv_.notify_all();
        }
        threads_.resize(thread_num);
        flags_.resize(thread_num);
      }
    }
#ifdef _DEBUG
    std::cerr << "Thread pool thread number changed to " << thread_num
              << std::endl;
#endif
  }

  /**
   * @brief pop a functional wrapper to the original function (without running
   * it)
   *
   * @return the wrapper
   */
  std::function<void(int)> pop() {
    std::function<void(int id)> *f_ptr = nullptr;
    q_.pop(f_ptr);
    // at return, delete the function
    std::unique_ptr<std::function<void(int id)>> func(f_ptr);
    std::function<void(int)> f;
    if (f_ptr) f = *f_ptr;
    return f;
  }

  /**
   * @brief Stop all threads
   *
   * @param isWait true if all the functions in the queue need to be run
   */
  void Stop(bool isWait = true) {
    if (!isWait) {
      if (is_stop_) return;
      is_stop_ = true;
      for (int i = 0, n = size(); i < n; i++) {
        *flags_[i] = true;  // command the threads to stop
      }
      ClearQueue();
    } else {
      if (is_done_ || is_stop_) return;
      is_done_ = true;  // command the threads to finish
    }
    {  // stop all waiting threads
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.notify_all();
    }
    for (int i = 0; i < static_cast<int>(threads_.size()); i++) {
      if (threads_[i]->joinable()) threads_[i]->join();
    }
    ClearQueue();
    threads_.clear();
    flags_.clear();
  }

  /**
   * @brief push a function into the queue (function's first param is the id of
   * the thread)
   *
   * @param f the function
   * @param rest the params
   * @return std::future of the function
   */
  template <typename F, typename... Rest>
  auto push(F &&f, Rest &&...rest) -> std::future<decltype(f(0, rest...))> {
    auto task_ptr =
        std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(
            std::bind(std::forward<F>(f), std::placeholders::_1,
                      std::forward<Rest>(rest)...));
    q_.push(new std::function<void(int id)>(
        [task_ptr](int id) { (*task_ptr)(id); }));
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.notify_one();
    }
    return task_ptr->get_future();
  }

  /**
   * @brief push a function into the queue (function's param is the id of
   * the thread)
   *
   * @param f the function
   * @return std::future of the function
   */
  template <typename F>
  auto push(F &&f) -> std::future<decltype(f(0))> {
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(
        std::forward<F>(f));
    q_.push(new std::function<void(int id)>(
        [task_ptr](int id) { (*task_ptr)(id); }));
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.notify_one();
    }
    return task_ptr->get_future();
  }
};
