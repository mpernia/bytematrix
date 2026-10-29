find_package(GTest REQUIRED)

add_executable(version_test
        version_test.cpp
        queue_test.cpp
        support_test.cpp
        generator_test.cpp
        filter_test.cpp
)

target_link_libraries(version_test PRIVATE
        bytematrix
        GTest::gtest_main)

target_compile_options(version_test PRIVATE -Wall -Wextra)

target_compile_definitions(version_test PRIVATE EXPECTED_VERSION="${PROJECT_VERSION}")

include(GoogleTest)
gtest_discover_tests(version_test)
