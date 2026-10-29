# bytematrix

A C++20 static library plus a demo consumer that models a three-stage byte-stream pipeline.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Docker](https://img.shields.io/badge/Container-Docker-2496ED.svg)](docker/Dockerfile)

bytematrix demonstrates a small concurrent pipeline built around a producer, a filtering stage, and a recorder sink. The library is designed to be reusable, while the demo app shows how it behaves end-to-end in a realistic consumer scenario.

## Table of contents

- [Overview](#overview)
- [Project structure](#project-structure)
- [Pipeline design](#pipeline-design)
- [Public API](#public-api)
- [Demo application](#demo-application)
- [Build and test](#build-and-test)
- [Examples](#examples)
- [Requirements](#requirements)
- [License](#license)

## Overview

The project is intentionally compact and educational:

- `libs/bytematrix` contains the reusable static library
- `apps/bmatrix` contains a demo application that consumes the library
- `docker/` provides the local development container used for building and testing
- the root `Makefile` orchestrates the workflow for contributors

The library exposes a generic module interface and a producer/filter/recorder pipeline that can be wired together by composition.

## Project structure

```text
bytematrix/
├── LICENSE
├── Makefile
├── README.md
├── apps/
│   └── bmatrix/
│       ├── CMakeLists.txt
│       ├── Makefile
│       ├── PipelineRunner.cpp
│       ├── PipelineRunner.hpp
│       ├── PipelineRunner_test.cpp
│       └── main.cpp
├── docker/
│   ├── Dockerfile
│   └── docker-compose.yml
└── libs/
    └── bytematrix/
        ├── CMakeLists.txt
        ├── Makefile
        ├── include/
        │   └── bytematrix.h
        ├── src/
        └── tests/
```

## Pipeline design

The library models a three-stage processing chain:

1. Generator
   - emits byte arrays of random length and content
   - continuously pushes data downstream

2. Filter
   - receives generated payloads
   - searches for a configured byte sequence inside each array
   - only forwards matching byte arrays

3. Recorder
   - receives the filtered payloads
   - stores the observed matching records
   - prints the accumulated history in time order

This setup illustrates a compact dataflow architecture with concurrency, internal buffering, and backpressure handling.

## Public API

The public interface is declared in `libs/bytematrix/include/bytematrix.h`.

```cpp
namespace bytematrix {

    std::string_view version() noexcept;

    inline constexpr std::size_t kInputBufferCapacity = 100;

    using ByteArray = std::vector<std::uint8_t>;

    class IModule {
    public:
        virtual ~IModule() = default;
        virtual void setNext(IModule* next) = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool submit(ByteArray data) = 0;
    };

    [[nodiscard]] std::unique_ptr<IModule> createGenerator();
    [[nodiscard]] std::unique_ptr<IModule> createFilter(ByteArray sequence);
    [[nodiscard]] std::unique_ptr<IModule> createRecorder();
}
```

Key characteristics:

- a module owns an internal worker thread
- stages can be chained through `setNext()`
- each stage buffers input with bounded capacity
- the library uses backpressure rather than uncontrolled memory growth

## Demo application

The demo app under `apps/bmatrix` creates a real runnable pipeline and executes it for a configurable duration.

It accepts:

- the number of seconds to run
- an optional target sequence as comma-separated hex bytes

Example:

```bash
./bmatrix 30 00,01,02
```

This runs for 30 seconds and looks for the sequence `00 01 02`.

The parser accepts values such as:

- `00,01,02`
- `0x0,0x1,0x2`

It rejects malformed inputs and prints usage information when arguments are invalid.

## Build and test

The repository is designed to be built inside a Docker development container. The root `Makefile` provides the high-level workflow.

### Core commands

```bash
make init
make build
make test
make tidy
make shell
```

### Lower-level commands

```bash
make exec CMD="ctest --test-dir build/test"
make exec CMD="cd libs/bytematrix && cmake -S . -B build && cmake --build build"
```

### Library project

```bash
cd libs/bytematrix
make test
make tidy-src
```

### Demo project

```bash
cd apps/bmatrix
make test
make tidy
```

## Examples

### Run the default demo

```bash
make build
./apps/bmatrix/build/bmatrix
```

### Run with a custom timeout and sequence

```bash
./apps/bmatrix/build/bmatrix 20 7f,00,ff
```

### Build and validate everything

```bash
make init
make test
```

## Requirements

- Docker
- GNU Make
- a Linux-like environment able to run the provided dev container
- C++20 toolchain inside the container

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Notes

This repository is intentionally small and focused. It aims to demonstrate a practical pattern for creating a reusable C++ library and a separate consumer application that integrates with it in a clean, maintainable structure.
