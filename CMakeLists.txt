cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES CXX)
find_package(GTest REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_CXX_FLAGS_DEBUG "-g")

# Enable testing globally
enable_testing()

include_directories("include/")

add_executable(tests
    tests/skip_map_test.cpp
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest_main
)

set_target_properties(tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exec
)

include(GoogleTest)
gtest_discover_tests(tests)