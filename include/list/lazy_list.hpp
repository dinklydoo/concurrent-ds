#pragma once
#include "rcu.hpp"
#include "set.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <iostream>

template<typename T>
struct lazy_list : public cset<T> {

private:
    enum node_flags : uint8_t {
        MARKED = (1 << 0),
        TAIL_SENTINEL = (1 << 1),
        HEAD_SENTINEL = (1 << 2)
    };
    struct lazy_node {
        std::atomic<lazy_node*> next;
        std::mutex lock;
        T val;
        std::atomic_uint8_t flag;

        lazy_node(const T& val) : next(nullptr), val(val), flag(0) {}
        lazy_node(bool cons, uint8_t flag) : next(nullptr), flag(flag) {}

        inline bool has_next(const T& nval) {
            // dont need to use atomics here
            uint8_t snapshot = flag.load(std::memory_order_acquire);

            if (snapshot & TAIL_SENTINEL) return false;
            return (val < nval);
        }

        inline bool equals(const T& elem) {
            if (flag.load(std::memory_order_acquire) & TAIL_SENTINEL) return false;
            return val == elem;
        }
    };
    
    inline bool validate(lazy_node* pred, lazy_node* curr){
        uint8_t pflag = pred->flag.load(std::memory_order_acquire);
        uint8_t cflag = curr->flag.load(std::memory_order_acquire);

        return !(pflag & MARKED) && !(cflag & MARKED) && pred->next.load(std::memory_order_acquire) == curr;
    }

    mutable RCU<lazy_node> rcu;
    lazy_node* head;
    lazy_node* tail;
public:
    lazy_list() : 
    rcu(),
    head(new lazy_node(0, HEAD_SENTINEL)), 
    tail(new lazy_node(0, TAIL_SENTINEL))
    { head->next = tail; }

    virtual void clear() override;
    virtual std::unordered_map<T, int> flatten() const override;
    virtual void print() const override;

    virtual void insert(const T&) override;
    virtual void erase(const T&) override;
    virtual bool contains(const T&) const override;

    virtual ~lazy_list() override {
        clear();
        delete head;
        delete tail;
    }
};

template<typename T>
bool lazy_list<T>::contains(const T& elem) const {
    rcu.enter();
    lazy_node* curr = head->next.load(std::memory_order::acquire);
    
    while (curr->has_next(elem)){
        curr = curr->next.load(std::memory_order_acquire);
    }

    bool res = (curr->equals(elem)) && !(curr->flag.load(std::memory_order_acquire) & MARKED);
    rcu.exit();
    return res;
}

template<typename T>
void lazy_list<T>::insert(const T& elem) {   
    rcu.enter();
    while (true) {

        // phase 1 : lockless read of list
        lazy_node* pred = head;
        lazy_node* curr = pred->next.load(std::memory_order_acquire);
        while (curr->has_next(elem)){
            pred = curr;
            curr = curr->next.load(std::memory_order_acquire);
        }

        // phase 2 : acquire critical locks -- blocking
        std::unique_lock<std::mutex> pred_lock(pred->lock);
        std::unique_lock<std::mutex> curr_lock(curr->lock);

        
        // phase 3 : locks acquired, validate list state
        if (validate(pred, curr)){
            if (curr->equals(elem)) break;
            
            // add node to list
            lazy_node* node = new lazy_node(elem);
            node->next.store(curr, std::memory_order_release);
            pred->next.store(node, std::memory_order_release);
            break;
        }
    }
    rcu.exit();
}

template<typename T>
void lazy_list<T>::erase(const T& elem) {
    rcu.enter();
    while (true) {

        // phase 1 : lockless read of list
        lazy_node* pred = head;
        lazy_node* curr = pred->next.load(std::memory_order_acquire);
        while (curr->has_next(elem)){
            pred = curr;
            curr = curr->next.load(std::memory_order_acquire);
        }

        // phase 2 : acquire critical locks -- blocking
        std::unique_lock<std::mutex> pred_lock(pred->lock);
        std::unique_lock<std::mutex> curr_lock(curr->lock);
        
        // phase 3 : locks acquired, validate list state
        if (validate(pred, curr)){
            if (curr->equals(elem)){
                curr->flag.fetch_or(MARKED, std::memory_order_acq_rel);
                pred->next.store(curr->next.load(std::memory_order_acquire), std::memory_order_release);
                rcu.retire(curr);
            }
            break;
        }
    }
    rcu.exit();
}


// NOTE : THESE METHODS ARE ONLY FOR TESTING AND ARE UNSAFE WHEN RUN MULTITHREADED

template<typename T>
void lazy_list<T>::clear() {
    lazy_node* node = head->next;
    while (!(node->flag & TAIL_SENTINEL)){
        lazy_node* temp = node->next;
        delete node;
        node = temp;
    }
}

template<typename T>
std::unordered_map<T,int> lazy_list<T>::flatten() const {
    std::unordered_map<T, int> res;

    lazy_node* node = head->next;
    while (!(node->flag & TAIL_SENTINEL)){
        res[node->val]++;
        node = node->next;
    }
    return res;
}

template<typename T>
void lazy_list<T>::print() const {
    lazy_node* node = head->next;
    while (!(node->flag & TAIL_SENTINEL)){
        std::cout << node->val << ' ';
        node = node->next;
    }
    std::cout << std::endl;
}