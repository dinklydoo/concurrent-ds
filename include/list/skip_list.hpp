#pragma once
#include "rcu.hpp"
#include "set.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#define LEVELS 4

template<typename T>
struct skip_list : public cset<T> {

private:
    enum node_flags : uint8_t {
        MARKED = (1 << 0),
        LINKED = (1 << 1),
        TAIL_SENTINEL = (1 << 2),
        HEAD_SENTINEL = (1 << 3)
    };

    
    struct skip_node {
        using next_list = std::vector<std::atomic<skip_node*>>;

        next_list next;
        std::mutex lock;
        std::atomic_uint8_t flag;
        T val;
        size_t level;

        skip_node(const T& val, size_t level) : 
            next(next_list(level, nullptr)), 
            val(val), flag(0), level(level) {}

        skip_node(bool cons, uint8_t flag) :
            next(next_list(LEVELS, nullptr)), 
            flag(flag | LINKED), level(level){}

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
    
    inline bool validate(skip_node* pred, skip_node* curr, size_t level){
        uint8_t pflag = pred->flag.load(std::memory_order_acquire);
        uint8_t cflag = curr->flag.load(std::memory_order_acquire);

        return !(pflag & MARKED) && !(cflag & MARKED) && pred->next[level].load(std::memory_order_acquire) == curr;
    }

    mutable RCU<skip_node> rcu;
    skip_node* head;
    skip_node* tail;
public:
    skip_list() : 
    rcu(),
    head(new skip_node(0, HEAD_SENTINEL)), 
    tail(new skip_node(0, TAIL_SENTINEL))
    { head->next = tail; }

    virtual void clear() override;
    virtual std::unordered_map<T, int> flatten() const override;
    virtual void print() const override;

    virtual void insert(const T&) override;
    virtual void erase(const T&) override;
    virtual bool contains(const T&) const override;

    virtual ~skip_list() override {
        clear();
        delete head;
        delete tail;
    }
};