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

  /**
   * @brief Set the Content Type
   *
   * @param content_type the new content type
   */
  void SetContentType(const std::string& content_type);

  /**
   * @brief Set the Content Length
   *
   * @param content_length the new content length
   */
  void SetContentLength(const uint64_t& content_length);

  /**
   * @brief Set the Body
   *
   * @param body the new body
   * @param content_length the length of the new body
   */
  void SetBody(const std::string&& body, const uint64_t& content_length);

  /**
   * @brief Set the Body from a file
   *
   * @param filepath the path of the file
   */
  void SetBody(const std::filesystem::path&& filepath);

 private:
  friend Router;

  static const std::string http_version_string;
  static const std::unordered_map<HttpStatusCode, std::string>
      http_status_code_string;

  std::string body;
  std::filesystem::path filepath;

  /**
   * @brief Send the HTTP Response to the socket
   *
   * @param server the server
   * @param status_code the HTTP status code
   * @param fd the file descriptor of the socket
   * @return whether the response was sent successfully
   */
  bool SendRequest(Server* const server, HttpStatusCode status_code,
                   const int& fd);
};

using HttpResponsePtr = std::unique_ptr<HttpResponse>;
