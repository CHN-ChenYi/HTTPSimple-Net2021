#pragma once

#include "ThreadPool.hpp"

template <typename Task, typename TaskResult>
class TaskQueue : public ThreadPool {
 public:
  TaskQueue() = delete;

  TaskQueue(const std::function<TaskResult(int, Task)> &task_handler)
      : task_handler_(task_handler) {}

  TaskQueue(const std::function<TaskResult(int, Task)> &task_handler,
            const int &thread_count)
      : task_handler_(task_handler_), ThreadPool(thread_count) {}

  /**
   * @brief push a new task into the queue
   *
   * @param task the new task
   * @return the future of the task handlers
   */
  auto push(const Task &task) { return ThreadPool::push(task_handler_, task); }

  /**
   * @brief Set the Task Handler
   *
   * @param task_handler function's first param is the id of the thread
   */
  void SetTaskHandler(
      const std::function<TaskResult(int, Task)> &task_handler) {
    task_handler_ = task_handler;
  }

 private:
  std::function<TaskResult(int, Task)> task_handler_;
};
