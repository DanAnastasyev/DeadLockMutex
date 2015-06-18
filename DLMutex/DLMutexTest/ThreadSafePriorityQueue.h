#pragma once
#include <vector>
#include <memory>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <dlMutex.h>

template <typename T>
class ThreadSafePriorityQueue {
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

    dlMutex vectorMutex;
    dlMutex sharedBlock;
	boost::condition_variable_any popCond;
	int popCount;
	std::vector<node> v;
public:
	ThreadSafePriorityQueue() {}

	void push(T val, int prior);
	bool pop(T& val);
	bool empty() {
        boost::unique_lock<dlMutex> vectorLock(vectorMutex);
		return v.empty();
	}
};

template <typename T>
void ThreadSafePriorityQueue<T>::push(T val, int prior) {
    boost::unique_lock<dlMutex> pushLock(sharedBlock);

    boost::unique_lock<dlMutex> vectorLock(vectorMutex);
	if (v.empty()) {
		v.push_back(node(val, prior));
		return;
	}

	v.push_back(node(val, prior));
	size_t i = v.size() - 1;
	
	vectorLock.unlock();

    boost::unique_lock<dlMutex> vectorSharedLock(vectorMutex);
    boost::unique_lock<dlMutex> lk(v[i].mut);

	while (i > 0) {
		boost::unique_lock<dlMutex> nextLk(v[parent(i)].mut);
		if (v[i].prior > v[parent(i)].prior) {
			swap(v[i], v[parent(i)]);
			i = parent(i);
			lk = std::move(nextLk);
		}
		else {
			break;
		}
	}
}

template <typename T>
bool ThreadSafePriorityQueue<T>::pop(T& val) {
    boost::unique_lock<dlMutex> popLock(sharedBlock);
	
    boost::unique_lock<dlMutex> lk;
    boost::unique_lock<dlMutex> vectorLock(vectorMutex);
	
	popCond.wait(vectorMutex, [&] {return v.empty() || v[0].mut.try_lock(); });
	if (v.empty()) {
		popCond.notify_one();
		return false;
	}
    lk = boost::unique_lock<dlMutex>(v[0].mut, boost::adopt_lock);
	if (v.size() == 1) {
		val = v[0].val;
		--popCount;
		v.pop_back();
		popCond.notify_one();
		return true;
	}
	
	boost::unique_lock<dlMutex> lock2(v[v.size() - 1].mut);
	val = v[0].val;
	v[0] = v[v.size() - 1];
	v.pop_back();
	lock2.unlock(); 

	vectorLock.unlock();
    boost::unique_lock<dlMutex> vectorSharedLock(vectorMutex);

	size_t i = 0;
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
			swap(v[i], v[max]);
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
		if (parent(l) == 0) {
			popCond.notify_one();
		}
	}
	popCond.notify_one();
	return true;
}