cmake_minimum_required(VERSION 3.25...3.31)

project(bytematrix VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bytematrix STATIC bytematrix.cpp)

target_compile_definitions(bytematrix PRIVATE BYTEMATRIX_VERSION="${PROJECT_VERSION}")
