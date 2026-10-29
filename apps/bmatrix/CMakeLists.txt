cmake_minimum_required(VERSION 3.25...3.31)

# The demo executable. The library keeps the `bytematrix` name.
project(bmatrix)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The demo consumes the library vendored by `make vendor`, not the installed
# copy: apps/bmatrix/libs/bytematrix/{bytematrix.h,libbytematrix.a}.
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/bytematrix)

if(NOT EXISTS ${VENDOR_DIR}/libbytematrix.a)
    message(FATAL_ERROR
            "vendored library not found in ${VENDOR_DIR}; run `make vendor` first")
endif()

add_executable(bmatrix main.cpp)

target_include_directories(bmatrix PRIVATE ${VENDOR_DIR})
target_link_libraries(bmatrix PRIVATE ${VENDOR_DIR}/libbytematrix.a)

target_compile_options(bmatrix PRIVATE -Wall -Wextra)
