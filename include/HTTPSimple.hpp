#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "TaskQueue.hpp"
#include "ThreadPool.hpp"

using ControllerFunc =
    std::function<void(const HttpRequestPtr&&,
                       std::function<void(const HttpResponsePtr&,
                                          const HttpStatusCode&)>&& callback)>;

class Server;

class Router : public TaskQueue<int, void> {
 public:
  Router(Server* const server);

  /**
   * @brief Set the thread number
   *
   * @param num the thread number
   */
  void SetThreadNum(const uint32_t& num);

  /**
   * @brief register a controller
   *
   * @param method the HTTP method
   * @param path the URL path
   * @param func the controller function
   */
  void RegisterController(const HttpMethod& method, std::string path,
                          const ControllerFunc&& func);
  void push(const int& fd);

 private:
  Server* const server_;
  std::unordered_map<std::string, ControllerFunc> controllers_;
};

class Server {
 public:
  Server();
  Logger logger;

  /**
   * @brief register a controller
   *
   * @param method the HTTP method
   * @param path the URL path
   * @param func the controller function
   */
  Server& RegisterController(const HttpMethod& method, const std::string& path,
                             const ControllerFunc&& func);

  /**
   * @brief Set the thread number
   *
   * @param num the thread number
   */
  Server& SetThreadNum(const uint32_t& num);

  /**
   * @brief Start the server (It's a blocking function)
   *
   * @param port the listening port
   */
  void Listen(const uint16_t& port);

 private:
  friend Router;
  friend HttpRequest;
  friend HttpResponse;

  static const int kMaxEpollEvents = 64;
  std::unique_ptr<Router> router_;
  std::unique_ptr<std::thread> task_parser_;
  std::unordered_map<int, std::string> client_addrs_;
};
