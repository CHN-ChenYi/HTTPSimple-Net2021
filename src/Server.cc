#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

#include "HTTPSimple.hpp"

Server::Server() { router_ = std::make_unique<Router>(this); }

Server& Server::RegisterController(const HttpMethod& method,
                                   const std::string& path,
                                   const ControllerFunc&& func) {
  router_->RegisterController(method, path, std::move(func));
  return *this;
}

Server& Server::SetThreadNum(const uint32_t& num) {
  router_->SetThreadNum(num);
  return *this;
}

void Server::Listen(const uint16_t& port) {
  // create
  int sockfd;
  sockaddr_in serverAddr;
  if (!~(sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
    std::stringstream ss;
    ss << "socket() failed! errno: " << errno;
    logger.Fatal(ss.str());
    exit(-1);
  }

  // bind
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  bzero(&(serverAddr.sin_zero), 8);
  if (!~bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
    std::stringstream ss;
    ss << "bind() failed! errno: " << errno;
    logger.Fatal(ss.str());
    close(sockfd);
    exit(-1);
  }

  // listen
  if (!~listen(sockfd, 5)) {
    std::stringstream ss;
    ss << "listen() failed! errno: " << errno;
    logger.Fatal(ss.str());
    close(sockfd);
    exit(-1);
  }

  std::stringstream ss;
  ss << "Listening on port " << port;
  logger.Info(ss.str());

  // init epoll listening thread
  const int epfd = epoll_create1(0);
  task_parser_ = std::make_unique<std::thread>([this, epfd]() {
    epoll_event events[kMaxEpollEvents];
    for (;;) {
      const int num_ready = epoll_wait(epfd, events, kMaxEpollEvents, -1);
      for (int i = 0; i < num_ready; i++) {
        if (events[i].events & EPOLLIN) {  // incoming request
          router_->push(events[i].data.fd);
        } else {  // encounter error
          close(events[i].data.fd);
          std::stringstream ss;
          ss << client_addrs_[events[i].data.fd] << " disconnected";
          logger.Info(ss.str());
          client_addrs_.erase(events[i].data.fd);
        }
      }
    }
  });

  // get connection
  for (;;) {
    int comfd;
    sockaddr_in clientAddr;
    auto socketaddr_size = sizeof(sockaddr_in);
    if (!~(comfd = accept(sockfd, reinterpret_cast<sockaddr*>(&clientAddr),
                          reinterpret_cast<socklen_t*>(&socketaddr_size)))) {
      std::stringstream ss;
      ss << "accept() failed! errno: " << errno;
      logger.Error(ss.str());
    }
    // log
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, addr, sizeof(addr));
    std::stringstream ss;
    ss << addr << ':' << ntohs(clientAddr.sin_port);
    client_addrs_[comfd] = ss.str();
    std::stringstream log_ss;
    log_ss << '[' << ss.str() << "] connected (fd = " << comfd << ")";
    logger.Info(log_ss.str());
    // set to non-blocking mode
    int flags;
    flags = fcntl(comfd, F_GETFL, 0);
    if (flags < 0) {
      std::stringstream log_ss;
      log_ss << '[' << ss.str() << "] fcntl(F_GETFL) failed (fd = " << comfd
             << ")";
      logger.Error(log_ss.str());
      close(comfd);
      client_addrs_.erase(comfd);
      continue;
    }
    if (fcntl(comfd, F_SETFL, flags | O_NONBLOCK) < 0) {
      std::stringstream log_ss;
      log_ss << '[' << ss.str() << "] fcntl(F_SETFL) failed (fd = " << comfd
             << ")";
      logger.Error(log_ss.str());
      close(comfd);
      client_addrs_.erase(comfd);
      continue;
    }
    // add to epoll list
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = comfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, comfd, &event);
  }
}
