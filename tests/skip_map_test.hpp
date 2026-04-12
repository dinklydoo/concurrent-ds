#include "skip_map.hpp"
#include <thread>
#include <unordered_map>

enum class QueryType {
    EMPLACE, 
    ERASE, 
    GET, 
    CONTAINS
};

template <typename K, typename V>
struct SkipQuery {
    QueryType type;
    K key;
    V value;
};

template <typename K, typename V>
void print(const SkipQuery<K,V>& query){
    auto id = std::this_thread::get_id();
    std::string out;
    switch (query.type) {
        case QueryType::EMPLACE : out += "emplace "; break;
        case QueryType::ERASE : out += "erase "; break;
        default : out += "other "; break;
    }
    std::cout << id << ':' << out << query.key << ' ' << query.value << std::endl;
}

template<typename K, typename V>
using query_list = std::vector<SkipQuery<K,V>>;

template<typename K, typename V>
void run_queries(SkipMap<K,V>& smap, const query_list<K,V>& list){
    for (auto& q : list) {
        switch (q.type) {
            case QueryType::EMPLACE :{
                smap.emplace(q.key, q.value);
                break;
            }
            case QueryType::ERASE :{
                smap.erase(q.key);
                break;
            }
            default : {

            }
        }
    }
}

template<typename K, typename V>
bool validate_skipmap(const SkipMap<K,V>& smap, const std::unordered_map<K, V>& expected) {
    node_ptr<K, V> curr = nullptr;
    // validate ordering and level consistency
    std::unordered_map<K, int> seen;
    for (int level = 0; level < SKIP_LEVELS; level++) {
        std::optional<K> prev;
        curr = smap.get_head(level);
        while (curr != nullptr) {
            if (!prev) prev = curr->pair.first;
            else {
                if (curr->pair.first < prev) {
                    std::cout << "ordering violation" << std::endl;
                    return false; // violates ordering
                }
                prev = curr->pair.first;
            }
            seen[curr->pair.first]++; // incr count

            if (curr->level <= level){
                std::cout <<level<< " level violation "<<curr->level<<' '<<curr->pair.first<<':'<<curr->pair.second << std::endl;
                return false;
            }
            curr = curr->succ[level];
        }
    }
    curr = smap.get_head(0);
    while (curr != nullptr){
        const K& key = curr->pair.first;
        if (!expected.empty()){
            if (!expected.contains(key)){
                std::cout << "unexpected key " << key << std::endl;
                return false;
            }
            if (expected.at(key) != curr->pair.second){
                std::cout << "unexpected value " << curr->pair.second << std::endl;
                return false;
            }
        }
        if (seen.at(key) > curr->level){
            std::cout << "level violation" << std::endl;
            return false;
        }
        curr = curr->get_succ(0);
    }

    return true;
}