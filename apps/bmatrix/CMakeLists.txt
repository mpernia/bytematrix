cmake_minimum_required(VERSION 3.25...3.31)

# The demo executable. The library keeps the `bytematrix` name.
project(bmatrix)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The demo consumes the library vendored by `make vendor`, not the installed
# copy: apps/bmatrix/libs/bytematrix/{bytematrix.h,libbytematrix.a}.
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/bytematrix)

if(NOT EXISTS ${VENDOR_DIR}/libbytematrix.a)
    message(FATAL_ERROR
            "vendored library not found in ${VENDOR_DIR}; run `make vendor` first")
endif()

add_executable(bmatrix main.cpp PipelineRunner.cpp)

target_include_directories(bmatrix PRIVATE ${VENDOR_DIR})
target_link_libraries(bmatrix PRIVATE ${VENDOR_DIR}/libbytematrix.a)

target_compile_options(bmatrix PRIVATE -Wall -Wextra)

include(CTest)

if(BUILD_TESTING)
    add_test(NAME BmatrixDemo.RunsAndReportsVersion COMMAND bmatrix 0)
    set_tests_properties(BmatrixDemo.RunsAndReportsVersion PROPERTIES
            PASS_REGULAR_EXPRESSION "libbytematrix [0-9]+\\.[0-9]+\\.[0-9]+")

    add_test(NAME BmatrixDemo.RejectsNonNumericDuration COMMAND bmatrix abc)
    set_tests_properties(BmatrixDemo.RejectsNonNumericDuration PROPERTIES
            PASS_REGULAR_EXPRESSION "duration must be a whole number of seconds")

    add_test(NAME BmatrixDemo.RejectsNegativeDuration COMMAND bmatrix -5)
    set_tests_properties(BmatrixDemo.RejectsNegativeDuration PROPERTIES
            PASS_REGULAR_EXPRESSION "duration must be non-negative")

    add_test(NAME BmatrixDemo.AcceptsASequenceArgument COMMAND bmatrix 1 00,01,02)
    set_tests_properties(BmatrixDemo.AcceptsASequenceArgument PROPERTIES
            PASS_REGULAR_EXPRESSION "finished after 1 s")

    add_test(NAME BmatrixDemo.RejectsAnInvalidSequence COMMAND bmatrix 1 zz)
    set_tests_properties(BmatrixDemo.RejectsAnInvalidSequence PROPERTIES
            PASS_REGULAR_EXPRESSION "Invalid sequence: expected comma-separated hex bytes")

    find_package(GTest REQUIRED)

    # PipelineRunner.cpp is compiled into both executables on purpose: a test can
    # then drive the class without the demo's main() and without a library target.
    add_executable(bmatrix_test PipelineRunner_test.cpp PipelineRunner.cpp)

    target_include_directories(bmatrix_test PRIVATE ${VENDOR_DIR})
    target_link_libraries(bmatrix_test PRIVATE ${VENDOR_DIR}/libbytematrix.a GTest::gtest_main)
    target_compile_options(bmatrix_test PRIVATE -Wall -Wextra)

    include(GoogleTest)
    gtest_discover_tests(bmatrix_test)
endif()
