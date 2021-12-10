#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "Logger.hpp"

enum HttpMethod {
  GET,
  POST,
  PUT,
  DELETE,
  HEAD,
  OPTIONS,
  TRACE,
  CONNECT,
  PATCH
};

class Server;
class Router;

struct HttpRequest {
 public:
  HttpMethod method;
  std::string path;
  std::unordered_map<std::string, std::string> params;
  std::unordered_map<std::string, std::string> headers;
  std::string body;

 private:
  friend Router;

  /**
   * @brief Parse a HTTP request from a socket
   *
   * @param remaining unprocessed bytes of the socket (the last recv() gets more
   * bytes than necessary)
   * @param server the server
   * @param fd the file descriptor of the socket
   * @return whether get a request successfully
   */
  bool parse(std::string &remaining, Server *const server, const int fd);
};

using HttpRequestPtr = std::unique_ptr<HttpRequest>;
