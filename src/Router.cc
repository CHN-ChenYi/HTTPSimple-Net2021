#include "HTTPSimple.hpp"

extern int errno;

Router::Router(Server* const server)
    : TaskQueue([this](int, int fd) {
        std::string remaining;
        HttpRequestPtr request = std::make_unique<HttpRequest>();
        while (request->parse(remaining, server_, fd)) {
          auto controller_key = request->path;
          controller_key.push_back(static_cast<char>(request->method));
          const auto controller = controllers_.find(controller_key);
          if (controller == controllers_.end()) {  // controller not found
            HttpResponse response;
            response.SetContentLength(0);
            response.SendRequest(server_, HttpStatusCode::NOT_FOUND,
                                 fd);  // return 404
          } else {                     // controller found
            controller->second(std::move(request),
                               [this, fd](const HttpResponsePtr& response,
                                          const HttpStatusCode& status_code) {
                                 response->SendRequest(server_, status_code,
                                                       fd);
                               });
          }
          request = std::make_unique<HttpRequest>();  // as the last request is
                                                      // processed, create a new
                                                      // request
        }
      }),
      server_(server) {}

void Router::SetThreadNum(const uint32_t& num) { TaskQueue::resize(num); }

void Router::RegisterController(const HttpMethod& method, std::string path,
                                const ControllerFunc&& func) {
  path.push_back(static_cast<char>(method));
  controllers_[path] = func;
}

void Router::push(const int& fd) { TaskQueue::push(fd); }
