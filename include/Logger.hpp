#pragma once

#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "TaskQueue.hpp"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

class Logger {
  std::unique_ptr<TaskQueue<const std::string&, void>> queue_;

 public:
  enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_OK,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
  };

  Logger() {
    queue_ = std::make_unique<TaskQueue<const std::string&, void>>(
        [](int, const std::string& str) { std::cout << str; });
  }

  void Log(const LogLevel& level, const std::string& message) {
    const auto now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << '[' << std::put_time(std::localtime(&now), "%F %T") << ']';
    switch (level) {
      case LOG_LEVEL_DEBUG:
        ss << " [" ANSI_COLOR_MAGENTA "DEBUG" ANSI_COLOR_RESET "] ";
        break;
      case LOG_LEVEL_INFO:
        ss << " [" ANSI_COLOR_BLUE "INFO" ANSI_COLOR_RESET "]  ";
        break;
      case LOG_LEVEL_OK:
        ss << " [" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "]    ";
        break;
      case LOG_LEVEL_WARN:
        ss << " [" ANSI_COLOR_YELLOW "WARN" ANSI_COLOR_RESET "]  ";
        break;
      case LOG_LEVEL_ERROR:
        ss << " [" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] ";
        break;
      case LOG_LEVEL_FATAL:
        ss << " [" ANSI_COLOR_RED "FATAL" ANSI_COLOR_RESET "] ";
        break;
    }
    ss << message << std::endl;

    queue_->push(ss.str());
  }

  void Debug(const std::string& message) { Log(LOG_LEVEL_DEBUG, message); }
  void Info(const std::string& message) { Log(LOG_LEVEL_INFO, message); }
  void Ok(const std::string& message) { Log(LOG_LEVEL_OK, message); }
  void Warn(const std::string& message) { Log(LOG_LEVEL_WARN, message); }
  void Error(const std::string& message) { Log(LOG_LEVEL_ERROR, message); }
  void Fatal(const std::string& message) { Log(LOG_LEVEL_FATAL, message); }
};
