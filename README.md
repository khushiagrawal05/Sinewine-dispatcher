# Sinewine-dispatcher
#  High-Performance, Thread-Safe Event Dispatcher in Modern C++

 Overview

This project implements a high-performance, thread-safe **Event Dispatcher** in modern C++ (C++17+). It's designed to efficiently manage and dispatch events with arguments across multiple handlers using strongly-typed enums and policy-based design principles.

Key Features

- **Strongly-Typed Events** using `enum class`
- **Thread-Safe** with `std::shared_mutex` or customizable locking
- **Supports Any Callable**: functions, lambdas, functors, etc.
- **Handler Prioritization**
- **Unregistration Support**
- **Benchmarking Suite** with 1 million event dispatches
- **Multithreaded Dispatch Test**
- **Modern C++ Design** using templates, `std::tuple`, `std::function`, `std::unique_ptr`, and `std::atomic`

---

##  Build Instructions

### Prerequisites

- C++17-compatible compiler (e.g., `g++ 9+`, `clang++ 10+`, or MSYS2 with MinGW-w64)
- CMake (optional for project scaffolding)

### Build & Run

Compile using:

```bash
g++ -std=c++17 -O2 -pthread -o event_dispatcher event_dispatcher.cpp
./event_dispatcher
