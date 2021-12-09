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

using HandlerFunc =
    std::function<void(const HttpRequestPtr&&,
                       std::function<void(const HttpResponsePtr&,
                                          const HttpStatusCode&)>&& callback)>;

class Server;

class Router : public TaskQueue<int, void> {
 public:
  Router(Server* const server);
  void SetThreadNum(const uint32_t& num);
  void RegisterHandler(const HttpMethod& method, std::string path,
                       const HandlerFunc&& func);
  void push(const int& fd);

 private:
  Server* const server_;
  std::unordered_map<std::string, HandlerFunc> handlers_;
};

class Server {
 public:
  Server();
  Logger logger;
  Server& RegisterHandler(const HttpMethod& method, const std::string& path,
                          const HandlerFunc&& func);
  Server& SetThreadNum(const uint32_t& num);
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
