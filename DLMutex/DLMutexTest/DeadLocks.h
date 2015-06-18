#pragma once
#include <vector>
#include <memory>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <dlMutex.h>

template <typename T>
class DeadLockQueue {
    struct node {
        dlMutex mut;
        T val;
        int prior;

        node(T _val, int _prior) : val(_val), prior(_prior) {}
        node(const node& n) : val(n.val), prior(n.prior) {}
        node& operator=(const node& n) {
            val = n.val;
            prior = n.prior;
            return *this;
        }
    };

    std::vector<node> v;
public:
    DeadLockQueue(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            v.push_back(node(i, 100 - i));
        }
    }

    void go(size_t i, bool dir);
};

template <typename T>
void DeadLockQueue<T>::go(size_t i, bool dirr) {
    boost::unique_lock<dlMutex> lock(v[i].mut);
    int dir = (dirr) ? 1 : -1;
    for (i += dir; (i >= 0 && i < v.size()); i += dir) {
        lock = boost::unique_lock<dlMutex>(v[i].mut);
    }
}