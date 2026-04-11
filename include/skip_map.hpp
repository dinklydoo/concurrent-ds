#pragma once
#include <mutex>
#include <unordered_map>
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
using node_ptr = Node<K,V>*;

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
        succ(level_vector<K,V>(level, nullptr)) {}
    ~Node() = default;

    node_ptr<K,V> get_succ(int l) {
        if (l >= level) return nullptr;
        return succ[l];
    }
};

template<typename K, typename V>
struct SkipMap {
private:
    level_vector<K,V> head;
    std::mutex head_lock;

    void copy_resources(const SkipMap& other);
    void move_resources(SkipMap& other);
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

    node_ptr<K,V> get_head(int level) const {return this->head[level]; }; // strictly for testing


    // TODO : test this LMAO
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
        next = (curr == nullptr)? this->head[level] : curr->get_succ(level);
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
    node_ptr<K,V> node = new Node<K,V>(key, value, rand_level);
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

            const K& next_key = (next->pair).first;

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

template<typename K, typename V>
void SkipMap<K,V>::erase(const K& key) {
    node_ptr<K,V> curr = nullptr;
    node_ptr<K,V> next = this->head[SKIP_LEVELS - 1];

    std::unique_lock<std::mutex> curr_lock(this->head_lock);

    node_ptr<K,V> erased = nullptr;
    std::unique_lock<std::mutex> erase_lock;
    
    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        if (curr == nullptr){
            next = this->head[level];
        }
        while (next != nullptr) {
            std::unique_lock<std::mutex> next_lock;
            if (next != erased) { // don't contend the lock
                next_lock = std::unique_lock<std::mutex>(next->lock);
            }

            const K& next_key = (next->pair).first;

            if (next_key < key){ // keep searching
                curr_lock.unlock();
                curr_lock = std::move(next_lock);
                curr = next;
                next = curr->succ[level];
                continue;
            }
            
            if (next_key == key){
                if (erased == nullptr){
                    erased = next;
                    erase_lock = std::move(next_lock);
                }
                node_ptr<K,V> nnext = next->succ[level];
                if (nnext != nullptr) next_lock = std::unique_lock<std::mutex>(nnext->lock);

                if (curr == nullptr) this->head[level] = nnext;
                else curr->succ[level] = nnext;
            }
            if (next_lock.owns_lock()) next_lock.unlock();
            break;
        }
        if (level > 0){
            if (curr == nullptr) next = this->head[level-1];
            else next = curr->succ[level-1];
        }
    }
    if (erased != nullptr) delete erased; // free memory

    curr_lock.unlock();
    if (erase_lock.owns_lock()) erase_lock.unlock(); // erase dne
}

template<typename K, typename V>
void SkipMap<K,V>::clear() {
    for (int level = SKIP_LEVELS - 1; level >= 0; level--){
        if (level == 0) {
            node_ptr<K,V> curr = this->head[0];
            while (curr != nullptr) {
                node_ptr<K,V> temp = curr;
                curr = curr->succ[0];
                delete temp;
            }
        }
        this->head[level] = nullptr;
    }
}

// TODO : make this nicer
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

template<typename K, typename V>
void SkipMap<K,V>::copy_resources(const SkipMap<K,V>& other) {
    std::unordered_map<K, node_ptr<K,V>> clone;
    node_ptr<K,V> curr = other.head[0];
    while (curr != nullptr){
        const K key = curr->get_key();
        const V value = curr->get_value();
        clone.emplace(key, new Node<K,V>(key, value, curr->level));

        curr = curr->succ[0];
    }
    for (int level = 0; level < SKIP_LEVELS; level++){
        curr = other.head[level];
        node_ptr<K,V> curr_copy = nullptr;
        if (curr != nullptr) curr_copy = clone.at(curr->get_key());

        this->head[level] = curr_copy;
        while (curr != nullptr){
            node_ptr<K,V> next = curr->get_succ(level);
            if (next != nullptr) {
                node_ptr<K,V> temp = clone.at(next->get_key());
                curr_copy->succ[level] = temp;

                curr = next;
                curr_copy = temp;
            }
            else {
                curr_copy->succ[level] = nullptr;
                break;
            }
        }
    }
}

template<typename K, typename V>
void SkipMap<K,V>::move_resources(SkipMap<K,V>& other) {
    node_ptr<K,V> curr;
    for (int level = 0; level < SKIP_LEVELS; level++){
        this->head[level] = other.head[level];
        other.head[level] = nullptr;
    }
}