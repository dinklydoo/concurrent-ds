#pragma once
#include <unordered_map>

template<typename T>
struct cset {
    virtual ~cset() = default;
    virtual void insert(const T&) = 0;
    virtual void erase(const T&) = 0;
    virtual bool contains(const T&) const = 0;


    // THESE METHODS BELOW FOR TESTING --> ASSUMED TO BE RUN SINGLE THREADED, NOT THREAD SAFE !!

    virtual void clear() = 0; 
    virtual void print() const = 0;
    virtual std::unordered_map<T, int> flatten() const = 0; // for testing purpose
};