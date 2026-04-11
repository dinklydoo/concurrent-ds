#include <gtest/gtest.h>
#include <thread>
#include <unordered_map>
#include "skip_map_test.hpp"

void insert_test_case1(SkipMap<int,int>& skip_map) {
    static query_list<int, int> queries1 = {
        {QueryType::INSERT, 1, 2},
        {QueryType::INSERT, 3, 5},
        {QueryType::INSERT, 4, 7},
        {QueryType::INSERT, 1, 8},
        {QueryType::INSERT, 3, 1}
    };

    static query_list<int, int> queries2 = {
        {QueryType::INSERT, 5, 4},
        {QueryType::INSERT, 2, 8},
        {QueryType::INSERT, 10, 6},
        {QueryType::INSERT, 10, 1},
        {QueryType::INSERT, 2, 2},
    };

    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries1));
    std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));

    t1.join();
    t2.join();
}

TEST(SKIP_MAP, INSERT){
    SkipMap<int, int> skip_map;
    insert_test_case1(skip_map);

    static std::unordered_map<int, int> expected1 {
        {1,8}, {2, 2}, {3, 1}, {4, 7}, {5, 4}, {10, 1}
    };
    EXPECT_TRUE(validate_skipmap(skip_map, expected1));
}

