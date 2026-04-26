#include <gtest/gtest.h>
#include <ostream>
#include <thread>
#include "set_test.hpp"
#include <unordered_set>

void insert_test_case1(set_ptr<int>& set) {
    static const query_list<int> queries1 = {
        {QueryType::INSERT, 1},
        {QueryType::INSERT, 2},
        {QueryType::INSERT, 3},
        {QueryType::INSERT, 4},
    };

    static const query_list<int> queries2 = {
        {QueryType::INSERT, 2},
        {QueryType::INSERT, 3},
        {QueryType::INSERT, 6},
        {QueryType::INSERT, 7},
    };

    std::thread t1(run_queries<int>, std::ref(set), std::ref(queries1));
    std::thread t2(run_queries<int>, std::ref(set), std::ref(queries2));

    t1.join();
    t2.join();
}

TEST(SET, INSERT){
    set_ptr<int> test_set = construct_set<int>();

    // TEST CASE 1
    insert_test_case1(test_set);

    static const std::unordered_set<int> expected1 {
        1, 2, 3, 4, 6, 7
    };

    EXPECT_TRUE(validate_set(test_set, expected1));
}

void erase_test_case1(set_ptr<float>& set){
    static const query_list<float> setup = {
        {QueryType::INSERT, 0.3},
        {QueryType::INSERT, 2.6},
        {QueryType::INSERT, 3.3},
        {QueryType::INSERT, 4.7},
        {QueryType::INSERT, 5.2},
    };

    run_queries(set, setup);

    static const query_list<float> queries1 = {
        {QueryType::ERASE, 0.6},
        {QueryType::ERASE, 2.6},
        {QueryType::ERASE, 3.3},
    };

    static const query_list<float> queries2 = {
        {QueryType::ERASE, 10.2},
        {QueryType::ERASE, 2.6},
        {QueryType::ERASE, 4.7},
        {QueryType::ERASE, 4.7},
    };

    std::thread t1(run_queries<float>, std::ref(set), std::ref(queries1));
    std::thread t2(run_queries<float>, std::ref(set), std::ref(queries2));

    t1.join();
    t2.join();
}

TEST(SET, ERASE){
    set_ptr<float> test_set = construct_set<float>();

    // TEST CASE 1
    erase_test_case1(test_set);

    static const std::unordered_set<float> expected1 {
        0.3, 5.2
    };

    EXPECT_TRUE(validate_set(test_set, expected1));
}

void mixed_test_case1(set_ptr<int>& set){
    static const query_list<int> queries1 = {
        {QueryType::INSERT, 1},
        {QueryType::INSERT, 2},
        {QueryType::INSERT, 3},
        {QueryType::INSERT, 4},
    };

    static const query_list<int> queries2 = {
        {QueryType::INSERT, 2},
        {QueryType::INSERT, 3},
        {QueryType::INSERT, 6},
        {QueryType::INSERT, 7},
    };

    static const query_list<int> queries3 = {
        {QueryType::ERASE, 1},
        {QueryType::ERASE, 2},
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 4},
    };

    static const query_list<int> queries4 = {
        {QueryType::ERASE, 2},
        {QueryType::ERASE, 3},
        {QueryType::ERASE, 6},
        {QueryType::ERASE, 7},
    };

    std::thread t1(run_queries<int>, std::ref(set), std::ref(queries1));
    std::thread t2(run_queries<int>, std::ref(set), std::ref(queries2));
    std::thread t3(run_queries<int>, std::ref(set), std::ref(queries3));
    std::thread t4(run_queries<int>, std::ref(set), std::ref(queries4));
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

TEST(SET, MIXED){
    set_ptr<int> test_set = construct_set<int>();

    // TEST CASE 1
    mixed_test_case1(test_set);

    EXPECT_TRUE(validate_set(test_set, {}));
}

void stress_test_case1(set_ptr<int>& set){
    query_list<int> queries1;
    for (int i = 0; i < 10000; i++){
        queries1.push_back({QueryType::INSERT, i});
    }

    query_list<int> queries2;
    for (int i = 0; i < 10000; i++){
        queries2.push_back({QueryType::ERASE, i});
    }

    std::thread t1(run_queries<int>, std::ref(set), std::ref(queries1));
    std::thread t2(run_queries<int>, std::ref(set), std::ref(queries2));
    
    t1.join();
    t2.join();
}

TEST(SET, STRESS){
    set_ptr<int> test_set = construct_set<int>();

    stress_test_case1(test_set);

    EXPECT_TRUE(validate_set(test_set, {}));
}