#include "HttpResponse.hpp"

#include <sys/socket.h>

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

#include "HTTPSimple.hpp"

using namespace std::chrono_literals;

const std::string HttpResponse::http_version_string = std::string("HTTP/1.1");
const std::unordered_map<HttpStatusCode, std::string>
    HttpResponse::http_status_code_string = {
        {HttpStatusCode::OK, std::string("200 OK")},
        {HttpStatusCode::BAD_REQUEST, std::string("400 Bad Request")},
        {HttpStatusCode::UNAUTHORIZED, std::string("401 Unauthorized")},
        {HttpStatusCode::FORBIDDEN, std::string("403 Forbidden")},
        {HttpStatusCode::NOT_FOUND, std::string("404 Not Found")},
        {HttpStatusCode::INTERNAL_SERVER_ERROR,
         std::string("500 Internal Server Error")},
        {HttpStatusCode::SERVICE_UNAVAILABLE,
         std::string("503 Service Unavailable")}};

void HttpResponse::SetContentType(const std::string& content_type) {
  headers[std::string("Content-Type")] = content_type;
}

void HttpResponse::SetContentLength(const uint64_t& content_length) {
  std::stringstream ss;
  ss << content_length;
  headers[std::string("Content-Length")] = ss.str();
}

void HttpResponse::SetBody(const std::string&& body,
                           const uint64_t& content_length) {
  filepath.clear();
  this->body = std::move(body);
  SetContentLength(content_length);
}

void HttpResponse::SetBody(const std::filesystem::path&& filepath) {
  SetContentLength(std::filesystem::file_size(filepath));
  this->filepath = std::move(filepath);
}

bool HttpResponse::SendRequest(Server* const server, HttpStatusCode status_code,
                               const int& fd) {
  // HTTP Status-Line
  std::stringstream ss;
  ss << http_version_string << ' ';
  ss << http_status_code_string.at(status_code) << "\r\n";

  // HTTP Header
  for (const auto& header : headers)
    ss << header.first << ": " << header.second << "\r\n";

  ss << "\r\n";

  // HTTP Content
  if (filepath.empty()) {  // content in memory
    ss << body;
  } else {  // content in file
    std::ifstream file(filepath);
    ss << file.rdbuf();
    file.close();
  }

  const auto response = ss.str();

  for (int i = 0; i < 10; i++) {  // try to send the response 10 times
    const auto ret = send(fd, reinterpret_cast<const void*>(response.data()),
                          response.size(), 0);
    if (ret == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {  // can't send now
        std::this_thread::sleep_for(200ms);
        continue;
      } else {  // encountered error
        std::stringstream error_ss;
        error_ss << '[' << server->client_addrs_[fd]
                 << "] send() failed, errno: " << errno;
        server->logger.Error(error_ss.str());
        return false;
      }
    } else {  // sent successfully
      return true;
    }
  }
  // sent failed after 10 retries
  std::stringstream error_ss;
  error_ss << '[' << server->client_addrs_[fd]
           << "] send() failed after 10 retries";
  server->logger.Error(error_ss.str());
  return false;
}
