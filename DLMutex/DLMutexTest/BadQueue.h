#pragma once
#include <vector>
#include <memory>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <dlMutex.h>

template <typename T>
class BadQueue {
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

    size_t parent(size_t idx) {
        return (idx - 1) / 2;
    }
    size_t left(size_t idx) {
        return (idx + idx + 1);
    }
    size_t right(size_t idx) {
        return (idx + idx + 2);
    }

    std::vector<node> v;
public:
    BadQueue(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            v.push_back(node(i, 100 - i));
        }
    }

    void up(size_t);
    void down(size_t);
};

template <typename T>
void BadQueue<T>::up(size_t i) {
    boost::unique_lock<dlMutex> lk(v[i].mut);

    while (i > 0) {
        lk = boost::unique_lock<dlMutex>(v[parent(i)].mut);
        i = parent(i);
    }
}

template <typename T>
void BadQueue<T>::down(size_t i) {
    boost::unique_lock<dlMutex> lk(v[i].mut);
    while (true) {
        size_t max = i, l = left(i), r = right(i);
        boost::unique_lock<dlMutex> lkL, lkR;

        if (l < v.size() && r < v.size()) {
            boost::lock(v[l].mut, v[r].mut);
            lkL = boost::unique_lock<dlMutex>(v[l].mut, boost::adopt_lock);
            lkR = boost::unique_lock<dlMutex>(v[r].mut, boost::adopt_lock);
        }
        else if (l < v.size()) {
            lkL = boost::unique_lock<dlMutex>(v[l].mut);
        }
        else {
            break;
        }

        if (l < v.size() && v[max].prior < v[l].prior) {
            max = l;
        }
        if (r < v.size() && v[max].prior < v[r].prior) {
            max = r;
        }
        if (max != i) {
            i = max;
            if (max == r) {
                lk = std::move(lkR);
            }
            else {
                lk = std::move(lkL);
            }
        }
        else {
            break;
        }
    }
}