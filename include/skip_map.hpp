#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <iostream>
#include <ostream>
#include <random>
#include <stdexcept>
#include <cassert>


#define SKIP_LEVELS 4

template<typename K, typename V>
struct Node;

template<typename K, typename V>
using node_ptr = std::shared_ptr<Node<K,V>>;

template<typename K, typename V>
using level_vector = std::vector<node_ptr<K,V>>;

template<typename K, typename V>
struct Node {
    std::pair<K, V> pair;
    std::mutex lock;

    const int level;
    level_vector<K, V> succ;

    Node(const K& key, const V& value, const int level) : 
        pair(std::pair(key, value)), 
        level(level), 
        succ(level_vector<K,V>(level, nullptr)) 
        {}

    V get_value() const;
    node_ptr<K,V> get_succ(int l) {
        if (l >= level) return nullptr;
        return succ[l];
    }
    void set_value(const V& value);
};


template<typename K, typename V>
struct SkipMap {
    level_vector<K,V> head;
    std::mutex head_lock;

    SkipMap() : head(level_vector<K, V>(SKIP_LEVELS, nullptr)) {}

    bool contains(const K& key) const;
    V get(const K& key) const;
    void emplace(const K& key, const V& value);
    void erase(const K& key);
    void print();


    // custom input iterator
    struct iterator {
        using self_type = iterator;
        using value_type = std::pair<K,V>;
        using reference = const std::pair<K,V>&;
        using pointer = const std::pair<K,V>*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        node_ptr<K,V> container;

        iterator(const node_ptr<K,V> c)
            : container(c) {}

        reference operator*() const {
            return container->pair;
        }
        pointer operator->() const {
            return &container->pair;
        }
        self_type& operator++() { // pre incr
            container = container->succ[0];
            return *this;
        }
        self_type operator++(int) { // post incr
            self_type tmp = *this;
            ++(*this);
            return tmp;
        }
        bool operator==(const self_type& other) const {
            return container == other.container;
        }
        bool operator!=(const self_type& other) const {
            return !(*this == other);
        }
    };
    iterator begin() const { return iterator(head[0]); }
    iterator end() const { return iterator(nullptr); }
};




template<typename K, typename V>
bool SkipMap<K,V>::contains(const K& key) const {
    node_ptr<K,V> curr = nullptr;
    node_ptr<K,V> next = this->head[SKIP_LEVELS - 1];

    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        next = (curr == nullptr)? this->head[level] : curr->get_succ(level);
        while (next != nullptr) {
            K& next_key = (next->pair).first;

            if (next_key < key){
                curr = next;
                next = curr->succ[level];
            }
            else if (next_key == key){
                return true;
            }
            else {
                break; // drop a level
            }
        }
    }
    return false;
}

template<typename K, typename V>
V SkipMap<K,V>::get(const K& key) const {
    node_ptr<K,V> curr = nullptr;
    node_ptr<K,V> next = this->head[SKIP_LEVELS - 1];

    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        next = (curr == nullptr)? this->head[level] : curr.get_succ(level);
        while (next != nullptr) {
            K& next_key = (next->pair).first;

            if (next_key < key){
                curr = next;
                next = curr->succ[level];
            }
            else if (next_key == key){
                return (next->pair).second;
            }
            else {
                break; // drop a level
            }
        }
    }
    throw std::out_of_range("key does not exist in skip-list");
}

// TODO : clean this up + ensure correctness with erase as an operation
template<typename K, typename V>
void SkipMap<K,V>::emplace(const K& key, const V& value) {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_int_distribution<> d(0, 1);

    int rand_level = 1;
    while (d(gen)) {
        rand_level++;
        if (rand_level == SKIP_LEVELS) break;
    }
    node_ptr<K,V> node = std::make_shared<Node<K,V>>(key, value, rand_level);
    if (contains(key)){
        node = nullptr;
        rand_level = 0;
    }
    node_ptr<K,V> curr = nullptr;
    node_ptr<K,V> next = this->head[SKIP_LEVELS - 1];
    
    std::unique_lock<std::mutex> curr_lock(this->head_lock);

    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        if (curr == nullptr && next == nullptr) { // empty layer
            if (rand_level - 1 >= level){
                this->head[level] = node;
            }
            else next = this->head[level];
        }
        while (next != nullptr) {
            std::unique_lock<std::mutex> next_lock(next->lock);

            K& next_key = (next->pair).first;

            if (next_key < key){
                curr_lock.unlock();
                curr_lock = std::move(next_lock); // hand over hand locking
                curr = next;
                next = curr->succ[level];
            }
            else if (next_key == key){
                (next->pair).second = value;
                curr_lock.unlock();
                next_lock.unlock();
                return;
            }
            else {
                if (rand_level-1 >= level) {
                    if (curr != nullptr) curr->succ[level] = node;
                    else this->head[level] = node;
                    node->succ[level] = next;
                }
                next_lock.unlock();
                if (level > 0){
                    next = (curr == nullptr)? this->head[level-1] : curr->succ[level-1];
                }
                break; // drop a level -> next will be non-null!!
            }
        }
        if (next == nullptr) {
            if (curr != nullptr){
                if (rand_level - 1 >= level) curr->succ[level] = node; // append at tail
                if (level > 0) next = curr->succ[level-1];
            }
            else {
                if (level > 0) next = this->head[level-1];
            }
        }
    }
    curr_lock.unlock();
}

// TODO : fix this
template<typename K, typename V>
void SkipMap<K,V>::erase(const K& key) {
    node_ptr<K,V> curr = nullptr;
    node_ptr<K,V> next = this->head[SKIP_LEVELS - 1];

    std::unique_lock<std::mutex> curr_lock(this->head_lock);

    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        
        while (next != nullptr) {
            std::unique_lock<std::mutex> next_lock(next->lock);
            K& next_key = (next->pair).first;

            if (next_key < key){
                curr_lock.unlock();
                std::swap(next_lock, curr_lock); // hand over hand locking
                curr = next;
                next = curr->succ[level];
            }
            else if (next_key == key){
                if (curr == nullptr) { // head is the erased node
                    this->head[level] = nullptr;
                    if (level > 0) next = this->head[level - 1];
                }
                else {
                    curr->succ[level] = next->succ[level];
                    next->succ[level] = nullptr;
                    next_lock.unlock();
                }
                break;
            }
            else {
                next_lock.unlock();
                break; // drop a level
            }
        }
    }
    curr_lock.unlock();
}

template<typename K, typename V>
void SkipMap<K,V>::print() {
    node_ptr<K,V> curr;
    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        curr = this->head[level];

        while (curr != nullptr ){
            std::cout << (curr->pair).first << ' ';
            curr = curr->succ[level];
        }
        std::cout << '\n';
    }
    std::flush(std::cout);
};