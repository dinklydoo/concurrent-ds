#include <gtest/gtest.h>
#include <thread>
#include <unordered_map>
#include "skip_map_test.hpp"

void emplace_test_case1(SkipMap<int,int>& skip_map) {
    static query_list<int, int> queries1 = {
        {QueryType::EMPLACE, 1, 2},
        {QueryType::EMPLACE, 3, 5},
        {QueryType::EMPLACE, 4, 7},
        {QueryType::EMPLACE, 1, 8},
        {QueryType::EMPLACE, 3, 1}
    };

    static query_list<int, int> queries2 = {
        {QueryType::EMPLACE, 5, 4},
        {QueryType::EMPLACE, 2, 8},
        {QueryType::EMPLACE, 10, 6},
        {QueryType::EMPLACE, 10, 1},
        {QueryType::EMPLACE, 2, 2},
    };

    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries1));
    std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));

    t1.join();
    t2.join();
}

void emplace_test_case2(SkipMap<int,int>& skip_map) {
    static const query_list<int, int> queries1 = {
        {QueryType::EMPLACE, 1, 11},
        {QueryType::EMPLACE, 3, 9},
    };

    static const query_list<int, int> queries2 = {
        {QueryType::EMPLACE, 6, 2},
        {QueryType::EMPLACE, 6, 3},
    };

    static const query_list<int, int> queries3 = {
        {QueryType::EMPLACE, 4, 11},
        {QueryType::EMPLACE, 2, 13},
    };

    static const query_list<int, int> queries4 = {
        {QueryType::EMPLACE, 5, 19},
        {QueryType::EMPLACE, 7, 1},
    };

    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries1));
    std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));
    std::thread t3(run_queries<int, int>, std::ref(skip_map), std::ref(queries3));
    std::thread t4(run_queries<int, int>, std::ref(skip_map), std::ref(queries4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

TEST(SKIP_MAP, EMPLACE){
    SkipMap<int, int> skip_map;

    // TEST CASE 1
    emplace_test_case1(skip_map);

    static const std::unordered_map<int, int> expected1 {
        {1, 8}, {2, 2}, {3, 1}, {4, 7}, {5, 4}, {10, 1}
    };
    EXPECT_TRUE(validate_skipmap(skip_map, expected1));

    // TEST CASE 2
    emplace_test_case2(skip_map);

    static const std::unordered_map<int, int> expected2 {
        {1, 11}, {2, 13}, {3, 9}, {4, 11}, {5, 19}, {6, 3}, {7,1}, {10, 1}
    };
    EXPECT_TRUE(validate_skipmap(skip_map, expected2));
}

void erase_test_case1(SkipMap<int,int>& skip_map) {
    static const query_list<int, int> queries1 = {
        {QueryType::EMPLACE, 1, 2},
        {QueryType::EMPLACE, 2, 8},
        {QueryType::EMPLACE, 3, 5},
        {QueryType::EMPLACE, 4, 7},
        {QueryType::EMPLACE, 5, 1}
    };

    static const query_list<int, int> queries2 = {
        {QueryType::ERASE, 2},
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 1},
        {QueryType::ERASE, 5},
        {QueryType::ERASE, 3},
    };

    run_queries(skip_map, queries1);
    run_queries(skip_map, queries2);
}

void erase_test_case2(SkipMap<int,int>& skip_map) {
    static const query_list<int, int> queries1 = {
        {QueryType::EMPLACE, 1, 1},
        {QueryType::EMPLACE, 2, 1},
        {QueryType::EMPLACE, 3, 1},
        {QueryType::EMPLACE, 4, 1},
        {QueryType::EMPLACE, 5, 1},
        {QueryType::EMPLACE, 6, 1},
        {QueryType::EMPLACE, 7, 1},
        {QueryType::EMPLACE, 8, 1},
    };

    static const query_list<int, int> queries2 = {
        {QueryType::ERASE, 2},
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 1},
        {QueryType::ERASE, 5},
        {QueryType::ERASE, 3},
    };

    static const query_list<int, int> queries3 = {
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 5},
        {QueryType::ERASE, 8},
        {QueryType::ERASE, 6},
    };
    // populate skip map
    run_queries(skip_map, queries1);
    
    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));
    std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries3));

    t1.join();
    t2.join();
}

TEST(SKIP_MAP, ERASE){
    SkipMap<int, int> skip_map;
    
    erase_test_case1(skip_map); // single-thread
    static const std::unordered_map<int, int> expected1 {
        {2, 8}, {4, 7}
    };
    EXPECT_TRUE(validate_skipmap(skip_map, expected1));

    skip_map.clear();
    erase_test_case2(skip_map);
    static const std::unordered_map<int, int> expected2 {
        {4, 1}, {7, 1}
    };
    EXPECT_TRUE(validate_skipmap(skip_map, expected2));
}

void mixed_test_case1(SkipMap<int,int>& skip_map) {
    static const query_list<int, int> queries1 = {
        {QueryType::EMPLACE, 1, 1},
        {QueryType::EMPLACE, 2, 1},
        {QueryType::EMPLACE, 3, 1},
        {QueryType::EMPLACE, 4, 1},
        {QueryType::EMPLACE, 5, 1},
        {QueryType::EMPLACE, 6, 1},
        {QueryType::EMPLACE, 7, 1},
        {QueryType::EMPLACE, 8, 1},
    };

    static const query_list<int, int> queries2 = {
        {QueryType::EMPLACE, 1, 2},
        {QueryType::EMPLACE, 2, 2},
        {QueryType::EMPLACE, 3, 2},
        {QueryType::EMPLACE, 8, 2},
    };

    static const query_list<int, int> queries3 = {
        {QueryType::ERASE, 2},
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 1},
        {QueryType::ERASE, 5},
        {QueryType::ERASE, 3},
    };

    static const query_list<int, int> queries4 = {
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 5},
        {QueryType::ERASE, 8},
        {QueryType::ERASE, 6},
    };

    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries1));
    std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));
    std::thread t3(run_queries<int, int>, std::ref(skip_map), std::ref(queries3));
    std::thread t4(run_queries<int, int>, std::ref(skip_map), std::ref(queries4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}


TEST(SKIP_MAP, MIXED){
    SkipMap<int, int> skip_map;

    mixed_test_case1(skip_map);
    EXPECT_TRUE(validate_skipmap(skip_map, {}));
}