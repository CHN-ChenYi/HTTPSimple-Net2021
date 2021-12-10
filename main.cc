#include <iostream>
#include <thread>

#include "./example/controllers.hpp"
#include "HTTPSimple.hpp"

int main(int argc, char** argv) {
  int port = 8080;
  if (argc == 2) {  // the first arg is the port number, default is 8080
    int tmp = std::stoi(argv[1]);
    if (0 <= tmp && tmp <= 65535) port = tmp;
  }

  const auto core_num = std::thread::hardware_concurrency();

  Server server;
  server.SetThreadNum(core_num ? core_num : 1)
      .RegisterHandler(HttpMethod::GET, "/", test)
      .RegisterHandler(HttpMethod::GET, "/noimg", noimg)
      .RegisterHandler(HttpMethod::GET, "/img/logo.jpg", img)
      .RegisterHandler(HttpMethod::POST, "/dopost", dopost)
      .Listen(port);
  return 0;
}
