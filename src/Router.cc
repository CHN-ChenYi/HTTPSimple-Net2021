#include "HTTPSimple.hpp"

extern int errno;

Router::Router(Server* const server)
    : TaskQueue([this](int, int fd) {
        std::string remaining;
        HttpRequestPtr request = std::make_unique<HttpRequest>();
        while (request->parse(remaining, server_, fd)) {
          auto handler_key = request->path;
          handler_key.push_back(static_cast<char>(request->method));
          const auto handler = handlers_.find(handler_key);
          if (handler == handlers_.end()) {
            HttpResponse response;
            response.SetContentLength(0);
            response.SendRequest(server_, HttpStatusCode::NOT_FOUND, fd);
          } else {
            handler->second(std::move(request),
                            [this, fd](const HttpResponsePtr& response,
                                       const HttpStatusCode& status_code) {
                              response->SendRequest(server_, status_code, fd);
                            });
          }
          request = std::make_unique<HttpRequest>();
        }
      }),
      server_(server) {}

void Router::SetThreadNum(const uint32_t& num) { TaskQueue::resize(num); }

void Router::RegisterHandler(const HttpMethod& method, std::string path,
                             const HandlerFunc&& func) {
  path.push_back(static_cast<char>(method));
  handlers_[path] = func;
}

void Router::push(const int& fd) { TaskQueue::push(fd); }
