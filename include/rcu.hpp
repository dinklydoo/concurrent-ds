#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <map>

// Simple portable epoch based RCU for deferred memory reclamation

// template with node types and call their user defined destructor methods

#define GRACE_PERIOD 2
#define EPOCH_FREQ 5
#define RECLAIM_FREQ 50

struct thread_epoch {
    std::atomic_uint64_t epoch_count;
    std::atomic_bool active;
};

template<typename node_T>
struct RCU {
private:
    std::atomic_uint64_t global_epoch{0};
    std::mutex reg_lock;
    std::vector<thread_epoch*> registry; // registry of active threads

    std::mutex ret_lock;
    std::map<uint64_t, std::vector<node_T*>> retired; // TODO make this generational/bucketed like Java -- less strict on ordering (sub ranges rather than by epoch)

    std::thread epoch_thread;
    std::atomic_bool active{true};

    thread_epoch* thread_store();
    void epoch_counter();

public:
    RCU() : epoch_thread([this]{ epoch_counter(); }) {}
    ~RCU() {
        // assume RCU destruction is called in a single thread
        // TODO : reclaim all memory from registry and retired
        
        active.store(false, std::memory_order_relaxed);
        epoch_thread.join();
    }

    void enter(); // reader or writer enters critical section -- could be reading lock free
    void exit();

    void retire(node_T*);
    void reclaim();
};

template<typename node_T>
void RCU<node_T>::epoch_counter() {
    std::atomic_uint64_t rc{0};

    while (active.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(EPOCH_FREQ));
        global_epoch.fetch_add(1, std::memory_order_acq_rel);
        rc.fetch_add(1, std::memory_order_acq_rel);

        if (rc.load(std::memory_order_acquire) % RECLAIM_FREQ == 0) reclaim();
    }
}

template<typename node_T>
thread_epoch* RCU<node_T>::thread_store() {
    thread_local thread_epoch* thread_state = nullptr;
    if (thread_state == nullptr){
        thread_state = new thread_epoch();
        thread_state->epoch_count.store(
            global_epoch.load(std::memory_order_acquire), 
            std::memory_order_release);
        thread_state->active.store(false);

        std::lock_guard<std::mutex> lock(reg_lock);
        registry.push_back(thread_state);
    }
    return thread_state;
}

template<typename node_T>
void RCU<node_T>::enter() {
    thread_epoch* thread_state = thread_store();
    thread_state->epoch_count.store(
        global_epoch.load(std::memory_order_acquire),
        std::memory_order_release
    );
    thread_state->active.store(true, std::memory_order_release);
}

template<typename node_T>
void RCU<node_T>::exit() {
    thread_epoch* thread_state = thread_store();
    thread_state->active.store(false, std::memory_order_release);
}

template<typename node_T>
void RCU<node_T>::retire(node_T* node){
    static std::atomic_uint32_t rc{0};
    uint64_t epoch = global_epoch.load(std::memory_order_acquire);

    std::lock_guard<std::mutex> lock(ret_lock);
    retired[epoch].push_back(node);
}

template<typename node_T>
void RCU<node_T>::reclaim(){
    std::lock_guard<std::mutex> reg(reg_lock);
    std::lock_guard<std::mutex> ret(ret_lock);

    uint64_t youngest = UINT64_MAX;
    for (auto it = registry.begin(); it != registry.end(); it++){
        if ((*it)->active.load(std::memory_order_acquire)){
            youngest = std::min(youngest, (*it)->epoch_count.load(std::memory_order_acquire));
        }
    }

    for (auto it = retired.begin(); it != retired.end(); ){
        auto& snapshot = *it;
        if (snapshot.first + GRACE_PERIOD <= youngest){
            for (node_T* node : snapshot.second){
                delete node;
            }
            it = retired.erase(it);
        }
        else it++;
    }
}

