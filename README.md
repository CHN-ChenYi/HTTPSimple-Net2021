# HTTPSimple-Net2021
A simple C++ backend framework -- A course lab for Computer Networks at ZJU.

## Features

* Decouple the network framework and controllers
* Use a network lib based on edge-triggered epoll, task queue and thread pool to provide high-concurrency, high-performance network IO
* Support asynchronous controllers to avoid blocking the main thread
