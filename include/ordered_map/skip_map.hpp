#pragma once
#include <mutex>
#include <vector>
#include <cassert>

#define SKIP_LEVELS 4

template<typename K, typename V>
struct Node;

template<typename K, typename V>
using node_ptr = Node<K,V>*;

template<typename K, typename V>
using level_vector = std::vector<node_ptr<K,V>>;



template<typename K, typename V>
struct SkipMap {
private:
    level_vector<K,V> head;
    std::mutex head_lock;

    void copy_resources(const SkipMap& other);
    void move_resources(SkipMap& other);

    struct Node {
        std::pair<K, V> pair;
        std::mutex lock;

        const int level;
        level_vector<K, V> succ;

        std::atomic_bool marked;
        std::atomic_bool linked;

        Node(const K& key, const V& value, const int level) : 
            pair(std::pair(key, value)), 
            level(level), 
            succ(level_vector<K,V>(level, nullptr)),
            marked(false),
            linked(true)
            {}

        ~Node() = default;

        node_ptr<K,V> get_succ(int l) {
            if (l >= level) return nullptr;
            return succ[l];
        }
    };
    
public:
    SkipMap() : head(level_vector<K, V>(SKIP_LEVELS, nullptr)) {}
    ~SkipMap() {
        this->clear(); // frees all nodes before cleanup
    }
    SkipMap(const SkipMap& other){ // copy cons -> deep copy on new defn
        copy_resources(other);
    }
    SkipMap& operator=(const SkipMap& other){ // copy assign -> deep copy on existing defn
        if (this != &other){
            this->clear(); // delete resources
            copy_resources(other);
        }
        return *this;
    }
    SkipMap(SkipMap&& other) noexcept { // move cons -> move resource to new defn
        move_resources(other);
    }
    SkipMap& operator=(SkipMap&& other) noexcept { // move assign -> move resource to exist defn
        if (this != &other){
            this->clear();
            move_resources(other);
        }
        return *this;
    }

    bool contains(const K& key) const;
    V get(const K& key) const;
    void emplace(const K& key, const V& value);
    void erase(const K& key);
    void clear();
    void print();
};