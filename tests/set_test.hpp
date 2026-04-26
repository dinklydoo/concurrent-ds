#include "set.hpp"
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "list/lazy_list.hpp"

template<typename T>
using set_ptr = std::unique_ptr<cset<T>>;

// Replace set interface to test different set implementations
template<typename T>
set_ptr<T> construct_set(){
    set_ptr<T> set = std::make_unique<lazy_list<T>>();
    return set;
}

enum class QueryType {
    INSERT,
    ERASE
};

template <typename T>
struct SetQuery {
    QueryType type;
    const T val;
};

template <typename T>
void print(const SetQuery<T>& query){
    auto id = std::this_thread::get_id();
    std::string out;
    switch (query.type) {
        case QueryType::INSERT : out += "insert "; break;
        case QueryType::ERASE : out += "erase "; break;
    }
    std::cout << id << ':' << out << query.val << std::endl;
}

template<typename T>
using query_list = std::vector<SetQuery<T>>;

template<typename T>
void run_queries(set_ptr<T>& set, const query_list<T>& list){
    for (auto& q : list) {
        switch (q.type) {
            case QueryType::INSERT :{
                set->insert(q.val);
                break;
            }
            case QueryType::ERASE :{
                set->erase(q.val);
                break;
            }
        }
    }
}

template<typename T>
bool validate_set(const set_ptr<T>& set, const std::unordered_set<T> expected){
    std::unordered_map<T, int> set_flat = set->flatten();
    for (auto& p : set_flat){
        if (p.second > 1){
            std::cerr << "unique property is not maintained, duplicate element: "<<p.first<<std::endl;
            return false;
        }
        if (!expected.empty() && !expected.count(p.first)){
            std::cerr << "unexpected element found in set: "<<p.first<<std::endl;
            return false;
        }
    }
    for (auto& elem : expected){
        if (!set_flat.count(elem)){
            std::cerr << "expected element not found in set: "<<elem<<std::endl;
            return false;
        }
    }
    return true; // ok
}