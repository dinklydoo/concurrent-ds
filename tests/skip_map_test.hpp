#include "skip_map.hpp"
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
bool validate_skipmap(const SkipMap<K,V>& smap, const std::unordered_map<K, V> expected) {
    node_ptr<K, V> curr = nullptr;
    // validate ordering and level consistency
    std::unordered_map<K, int> seen;
    for (int level = SKIP_LEVELS - 1; level >= 0; level--) {
        std::optional<K> prev;
        curr = smap.head[level];
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

            if (level == 0) { 
                const K& key = curr->pair.first;
                if (seen[key] != curr->level){
                    std::cout << "level violation" << std::endl;
                    return false;
                }
                if (!expected.contains(key)){
                    std::cout << "unexpected key " << key << std::endl;
                    return false;
                }
                if (expected.at(key) != curr->pair.second){
                    std::cout << "unexpected value " << curr->pair.second << std::endl;
                    return false;
                }
            }
            curr = curr->succ[level];
        }
    }
    return true;
}