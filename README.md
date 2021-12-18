# HTTPSimple-Net2021
A simple C++ backend framework -- A course lab for Computer Networks at ZJU.

## Features

* Decouple the network framework and controllers
* Use a network lib based on edge-triggered epoll, task queue and thread pool to provide high-concurrency, high-performance network IO
* Support asynchronous controllers to avoid blocking the main thread

## Hello World Example

``` c++
#include "HTTPSimple.hpp"

void hello_world_controller(
    const HttpRequestPtr&&,
    std::function<void(const HttpResponsePtr&,
                       const HttpStatusCode& status_code)>&& callback) {
  HttpResponsePtr resp = std::make_unique<HttpResponse>();
  resp->SetContentType("text/html");
  const std::string s("Hello World!");
  resp->SetBody(std::move(s), s.size());
  callback(resp, HttpStatusCode::OK);
};

int main() {
  Server server;
  server.RegisterController(HttpMethod::GET, "/hello", hello_world_controller)
      .Listen(8080);

  return 0;
}

```

For more examples, please refer to the `example` folder and `main.cc`.

## Compile and Run

Requires C++17.

``` Bash
cmake -B ./build -DCMAKE_BUILD_TYPE=Release .
make -C ./build -j
./build/HTTPSimple
```
