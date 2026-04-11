#include <gtest/gtest.h>
#include <thread>
#include "skip_map.hpp"

enum class QueryType {
    INSERT, DELETE, GET, CONTAINS
};

template <typename K, typename V>
struct SkipQuery {
    QueryType type;
    K key;
    V value;
};

template<typename K, typename V>
using query_list = std::vector<SkipQuery<K,V>>;


// TEST(AddTest, AddWorks) {
//     EXPECT_EQ(2+3, 5);
// }
template<typename K, typename V>
void run_queries(SkipMap<K,V>& smap, query_list<K,V>& list){
    for (auto& q : list) {
        switch (q.type) {
            case QueryType::INSERT :{
                smap.emplace(q.key, q.value);
                break;
            }
            case QueryType::DELETE :{
                smap.erase(q.key);
                break;
            }
            default : {

            }
        }
        smap.print();
        std::cout << std::endl;
    }
}

void test_case1() {
    SkipMap<int, int> skip_map;

    static query_list<int, int> queries1 = {
        {QueryType::INSERT, 1, 2},
        {QueryType::INSERT, 3, 5},
        {QueryType::INSERT, 4, 7},
        // {QueryType::DELETE, 1},
        // {QueryType::DELETE, 4},
        // {QueryType::DELETE, 3},
        {QueryType::INSERT, 1, 8},
        {QueryType::INSERT, 3, 1}
    };

    static query_list<int, int> queries2 = {
        {QueryType::INSERT, 5, 4},
        {QueryType::INSERT, 2, 8},
        {QueryType::INSERT, 10, 6},
        {QueryType::INSERT, 10, 1},
        // {QueryType::DELETE, 2},
        // {QueryType::DELETE, 5}
    };

    std::thread t1(run_queries<int, int>, std::ref(skip_map), std::ref(queries1));
    // std::thread t2(run_queries<int, int>, std::ref(skip_map), std::ref(queries2));

    t1.join();
    // t2.join();

    skip_map.print();
}


int main() {
    test_case1();
}