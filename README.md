# bytematrix

A three-stage byte-stream pipeline packaged as a static C++ library and a demo application that exercises it end-to-end.

bytematrix models a simple producer/filter/recorder pipeline:

- Generator: emits byte arrays of random length and content.
- Filter: keeps only payloads that contain a configured byte sequence.
- Recorder: receives matching payloads and logs the observed records in time order.

The project is organized as a small multi-project CMake workspace built and tested inside a Dockerized development environment.

## Repository layout

- `libs/bytematrix/`: static library implementation and tests
- `apps/bmatrix/`: demo consumer application that builds against the library
- `docker/`: Docker setup for the dev container
- `Makefile`: root orchestration for build, test, and tooling tasks

## Library API

The public interface is exposed through `libs/bytematrix/include/bytematrix.h`.

The library provides:

- `bytematrix::IModule`: abstract pipeline stage interface
- `bytematrix::createGenerator()`: creates the byte source module
- `bytematrix::createFilter(ByteArray sequence)`: creates a matching filter stage
- `bytematrix::createRecorder()`: creates the sink that captures matching payloads
- `bytematrix::ByteArray`: byte vector type used across the pipeline

The input/output contracts are intentionally simple:

- each stage can receive one payload at a time
- each stage has an internal worker thread
- the pipeline uses bounded buffering and backpressure to avoid unbounded memory growth

## Demo application

The demo lives under `apps/bmatrix` and drives a real pipeline instance using a configurable runtime and target sequence.

Example usage:

```bash
./bmatrix 30 00,01,02
```

This runs the demo for 30 seconds while searching for the byte sequence `00 01 02`.

The application supports:

- duration in seconds
- sequence parsing from comma-separated hex values such as `00,01,02` or `0x0,0x1,0x2`
- graceful error handling for invalid arguments

## Build and test workflow

This repository uses Docker + Make as the primary developer workflow. The root `Makefile` starts a containerized environment and delegates build/test commands to each subproject.

Common commands:

```bash
make init
make build
make test
make tidy
make shell
```

Useful lower-level commands:

```bash
make exec CMD="ctest --test-dir build/test"
make exec CMD="cd libs/bytematrix && cmake -S . -B build && cmake --build build"
```

## How the pipeline works

1. The generator produces a stream of random byte arrays.
2. The filter checks whether the configured byte sequence appears in each array.
3. Matching payloads are passed to the recorder, which records them and prints the history seen so far.

This creates a simplified but realistic producer-consumer architecture with concurrency, buffering, and incremental observation.

## Requirements

- Docker
- GNU Make
- A Linux-like environment capable of running the provided development container

## License

This project is distributed under the MIT License. See `LICENSE` for details.

## Notes

The repository is intentionally structured as a small C++20 learning project and demo system: it keeps the library and consumer application separated, uses static linking, and demonstrates how to build a reusable library plus a concrete application around it.
