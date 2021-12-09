#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

enum class HttpStatusCode {
  OK = 200,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  INTERNAL_SERVER_ERROR = 500,
  SERVICE_UNAVAILABLE = 503
};

class Router;
class Server;

struct HttpResponse {
 public:
  std::unordered_map<std::string, std::string> headers;
  void SetContentType(const std::string& content_type);
  void SetContentLength(const uint64_t& content_length);
  void SetBody(const std::string&& body, const uint64_t& content_length);
  void SetBody(const std::filesystem::path&& filepath);

 private:
  friend Router;

  static const std::string http_version_string;
  static const std::unordered_map<HttpStatusCode, std::string>
      http_status_code_string;

  std::string body;
  std::filesystem::path filepath;

  bool SendRequest(Server* const server, HttpStatusCode status_code,
                   const int& fd);
};

using HttpResponsePtr = std::unique_ptr<HttpResponse>;
