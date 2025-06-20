# hello-grpc: A Go/C++ Hybrid RISC-V Emulator Service
# hello-grpc: 一个 Go/C++ 混合实现的 RISC-V 仿真器服务

This project provides a high-performance RISC-V emulator accessible via a gRPC interface. It features a Go-based gRPC server that interfaces with a C++ emulation core using CGo, combining the ease of use of Go for networking with the performance of C++ for computation-intensive tasks.

本项目提供了一个通过 gRPC 接口访问的高性能 RISC-V 仿真器。其特点是采用 Go 语言编写的 gRPC 服务器，通过 CGo 与 C++ 仿真核心进行交互，从而将 Go 在网络编程方面的易用性与 C++ 在计算密集型任务上的高性能结合起来。

## Features
## 功能特性

- **gRPC Interface**: Control the emulator, read/write memory, set breakpoints, and access registers over a modern RPC framework.
  - **gRPC 接口**: 通过现代化的 RPC 框架控制仿真器、读写内存、设置断点和访问寄存器。
- **CGo-based Hybrid Stack**: Leverages a C++ core for emulation performance and a Go server for robust network services.
  - **基于 CGo 的混合技术栈**: 利用 C++ 核心提供仿真性能，并采用 Go 服务器提供稳健的网络服务。
- **Dynamic Core Implementation**: Uses Go build tags (`cgo`) to switch between a real CGo-powered core and a dummy core for testing or platform compatibility.
  - **动态核心实现**: 使用 Go 的构建标签 (`cgo`) 来在真实的 CGo 核心和用于测试或平台兼容性的"虚拟"核心之间切换。
- **Comprehensive Test Suite**: Includes end-to-end gRPC integration tests and unit tests for the CGo bridge.
  - **全面的测试套件**: 包括端到端的 gRPC 集成测试和针对 CGo 桥接层的单元测试。
- **Structured Logging**: Uses `zerolog` for structured, leveled logging of gRPC requests.
  - **结构化日志**: 使用 `zerolog` 对 gRPC 请求进行结构化、分级别的日志记录。

## Dependencies
## 环境依赖

To build and run this project, you will need the following tools installed:
要构建和运行此项目，您需要安装以下工具：

- **Go**: Version 1.18 or later. / 1.18 或更高版本。
- **CMake**: Version 3.16 or later. / 3.16 或更高版本。
- **g++**: A C++11 compatible compiler. / 兼容 C++11 的编译器。
- **Make**

## Quick Start
## 快速上手

Follow these steps to build the C++ core, the Go server, and run the tests.
请按照以下步骤构建 C++ 核心、Go 服务器并运行测试。

### 1. Build the C++ Core
### 1. 构建 C++ 核心库

First, compile the C++ emulator core into a static library (`libvemu.a`).
首先，将 C++ 仿真器核心编译成一个静态库 (`libvemu.a`)。

```bash
# Navigate to the C++ service directory
# 导航到 C++ 服务目录
cd vemu_service

# Create a build directory
# 创建一个构建目录
mkdir -p build && cd build

# Configure the project with CMake and build it
# 使用 CMake 配置项目并构建
cmake ..
make

# Return to the project root
# 返回项目根目录
cd ../..
```

The static library will be generated in the `vemu_service/build/` directory.
静态库文件将在 `vemu_service/build/` 目录下生成。

### 2. Build the Go gRPC Server
### 2. 构建 Go gRPC 服务器

With the C++ library in place, you can now build the Go server. The `cgo` build tag is required to link against the C++ core.
在 C++ 库就绪后，您现在可以构建 Go 服务器。构建时需要 `cgo` 标签以链接 C++ 核心。

```bash
# Navigate to the Go module directory
# 导航到 Go 模块目录
cd go-vemu

# Build the server binary
# 构建服务器可执行文件
go build -tags cgo ./cmd/server
```

The executable `server` will be created in the `go-vemu/` directory.
可执行文件 `server` 将在 `go-vemu/` 目录下创建。

### 3. Run Tests
### 3. 运行测试

To verify that everything is working correctly, run the full test suite. This includes Go unit tests, CGo bridge tests, and gRPC integration tests.
为了验证一切是否正常，请运行完整的测试套件。这包括 Go 单元测试、CGo 桥接测试和 gRPC 集成测试。

```bash
# Make sure you are in the go-vemu directory
# 确保您位于 go-vemu 目录下
# cd go-vemu

# Run all tests with CGO enabled
# 启用 CGO 运行所有测试
go test -v -tags cgo ./...
```

All tests should pass, confirming that the Go and C++ parts are integrated correctly.
所有测试都应该通过，这确认了 Go 和 C++ 部分已正确集成。

## gRPC API

The service API is defined in Protocol Buffers in the following file:
服务接口在以下 Protocol Buffers 文件中定义：
`go-vemu/api/v1/vemu_service.proto`

You can use tools like `grpcurl` to explore and interact with the API endpoints once the server is running.
服务器运行后，您可以使用 `grpcurl` 等工具来探索和交互 API 端点。