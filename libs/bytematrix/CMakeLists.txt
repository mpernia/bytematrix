cmake_minimum_required(VERSION 3.25...3.31)

project(bytematrix VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(bytematrix STATIC
        src/ByteGenerator.cpp
        src/bytematrix.cpp
        src/QueuedModule.cpp
        src/SequenceFilter.cpp
        src/ByteArrayRecorder.cpp
)

target_include_directories(bytematrix PUBLIC include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bytematrix PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wunused)
endif()
target_compile_definitions(bytematrix PRIVATE BYTEMATRIX_VERSION="${PROJECT_VERSION}")

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
