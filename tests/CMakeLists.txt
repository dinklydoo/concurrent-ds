find_package(GTest REQUIRED)

add_executable(tests
    skip_map_test.cpp
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)