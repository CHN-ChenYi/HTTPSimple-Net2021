#include "HttpRequest.hpp"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#include "HTTPSimple.hpp"
using namespace std::chrono_literals;

static const int kBufferSize = 65535;

// TODO: decode url
bool HttpRequest::parse(std::string &remaining, Server *const server,
                        const int fd) {
  char recv_buffer[kBufferSize], process_buffer[kBufferSize];

  int size_left_from_socket;
  ioctl(fd, FIONREAD, &size_left_from_socket);
  int64_t recv_cnt = remaining.length(), new_recv_cnt = 0;
  if (size_left_from_socket)
    recv_cnt += new_recv_cnt = recv(fd, recv_buffer, kBufferSize, 0);
  if (recv_cnt < 0) {
    std::stringstream ss;
    ss << '[' << server->client_addrs_[fd]
       << "] Request line recv() failed, errno: " << errno << std::endl;
    server->logger.Error(ss.str());
    close(fd);
    return false;
  } else if (recv_cnt == 0) {
    return false;
  }
  std::string real_recv_buffer(recv_buffer, new_recv_cnt);
  std::stringstream recv_ss;
  recv_ss << remaining << real_recv_buffer;

  // Request-Line
  recv_ss.getline(process_buffer, kBufferSize);
  std::stringstream proc_ss(process_buffer);
  std::string method, path, version;
  proc_ss >> method >> path >> version;
  std::stringstream info_ss;
  info_ss << '[' << server->client_addrs_[fd] << "] " << method << " " << path
          << " ";
  // method
  if (method == "GET") {
    this->method = HttpMethod::GET;
  } else if (method == "POST") {
    this->method = HttpMethod::POST;
  } else if (method == "PUT") {
    this->method = HttpMethod::PUT;
  } else if (method == "DELETE") {
    this->method = HttpMethod::DELETE;
  } else if (method == "HEAD") {
    this->method = HttpMethod::HEAD;
  } else if (method == "OPTIONS") {
    this->method = HttpMethod::OPTIONS;
  } else if (method == "TRACE") {
    this->method = HttpMethod::TRACE;
  } else if (method == "CONNECT") {
    this->method = HttpMethod::CONNECT;
  } else if (method == "PATCH") {
    this->method = HttpMethod::PATCH;
  } else {
    std::stringstream ss;
    ss << '[' << server->client_addrs_[fd] << "] Unknown method: " << method;
    server->logger.Error(ss.str());
    close(fd);
    return false;
  }
  // path
  this->path = path;  // TODO: params
  // version
  if (version != std::string("HTTP/1.1")) {
    std::stringstream ss;
    ss << '[' << server->client_addrs_[fd]
       << "] Unknown HTTP version: " << version;
    server->logger.Error(ss.str());
    return false;
  }

  // HTTP Header
  while (recv_ss.getline(process_buffer, kBufferSize)) {
    if (process_buffer[0] == '\r') {
      break;  // Header Stopped
    }
    auto colon_pos = strstr(process_buffer, ": ");
    this->headers[std::string(process_buffer, colon_pos - process_buffer)] =
        std::string(colon_pos + 2,
                    strlen(process_buffer) - (colon_pos + 2 - process_buffer) -
                        1);  // remove \r
  }

  // body
  if (this->headers.count("Transfer-Encoding") ||
      !this->headers.count("Content-Length")) {
    if (this->method == HttpMethod::GET) {
      this->body = "";
      const auto remaining_length = recv_cnt - recv_ss.tellg();
      char remaining_buffer[remaining_length];
      recv_ss.readsome(remaining_buffer, remaining_length);
      remaining = std::string(remaining_buffer, remaining_length);
      server->logger.Info(info_ss.str());
      return true;
    }
    std::stringstream ss;
    ss << '[' << server->client_addrs_[fd]
       << "] Unknown Transfer-Encoding or Content-Length";
    server->logger.Error(ss.str());
    close(fd);
    return false;
  }
  int64_t content_length = std::stoll(this->headers["Content-Length"]);
  if (recv_cnt - recv_ss.tellg() >= content_length) {
    char buf[content_length + 1];
    recv_ss.readsome(buf, content_length);
    buf[content_length] = '\0';
    this->body = buf;
    const auto remaining_length = recv_cnt - recv_ss.tellg();
    char remaining_buffer[remaining_length];
    recv_ss.readsome(remaining_buffer, remaining_length);
    remaining = std::string(remaining_buffer, remaining_length);
    server->logger.Info(info_ss.str());
    return true;
  }
  recv_ss >> this->body;
  content_length -= this->body.length();
  recv_cnt = new_recv_cnt = 0;
  while (content_length > 0) {
    const auto recv_chunk_size = std::min<size_t>(content_length, kBufferSize);
    for (int i = 0; i < 10; i++) {
      ioctl(fd, FIONREAD, &size_left_from_socket);
      if (size_left_from_socket) break;
      std::this_thread::sleep_for(200ms);
    }
    if (size_left_from_socket == 0) {
      std::stringstream ss;
      ss << '[' << server->client_addrs_[fd]
         << "] Request body shorter than expected: " << content_length;
      server->logger.Error(ss.str());
      close(fd);
      return false;
    }
    recv_cnt = recv(fd, recv_buffer, recv_chunk_size, 0);
    recv_buffer[recv_cnt] = '\0';
    new_recv_cnt = std::min<size_t>(recv_cnt, content_length);
    if (recv_cnt < 0) {
      std::stringstream ss;
      ss << '[' << server->client_addrs_[fd]
         << "] Content recv() failed, errno: " << errno;
      server->logger.Error(ss.str());
      close(fd);
      return false;
    }
    this->body += std::string(recv_buffer, new_recv_cnt);
    content_length -= new_recv_cnt;
  }

  remaining = std::string(recv_buffer + new_recv_cnt, recv_cnt - new_recv_cnt);

  server->logger.Info(info_ss.str());
  return true;
}
