#pragma once
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <future>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include "ThreadPool.h"

ThreadPool::ThreadPool() : init(false) {}

void ThreadPool::work() {
    while (true) {
        try {
            boost::unique_lock<boost::mutex> lock(queue_mutex);
            workCondition.wait(lock, [&]() { return !tasks.empty() || stop || wait; });
            if (tasks.empty()) {
                if (stop) {
                    return;
                }
                if (wait) {
                    ++waitCount;
                    waitMainThreadCondition.notify_one();
                    waitCondition.wait(lock, [&]() { return !wait; });
                    continue;
                }
            }
            std::function<void()> task(tasks.front());
            tasks.pop();
            lock.unlock();

            task();
        }
        catch (boost::thread_interrupted&) {
        }
    }
}

void ThreadPool::Init(size_t threads) {
    if (init) {
        throw std::exception("ThreadPool already initialized");
    }
    init = true;
    stop = false;
    wait = false;
    try {
        for (size_t i = 0; i < threads; ++i) {
            workers.push_back(std::make_shared<boost::thread>(&ThreadPool::work, this));
        }
    }
    catch (...) {
        stop = true;
        for (size_t i = 0; i < workers.size(); ++i) {
            if (workers[i]->joinable()) {
                workers[i]->join();
            }
        }
        throw;
    }
}

ThreadPool::ThreadPool(size_t threads) : init(true), stop(false), wait(false) {
    try {
        for (size_t i = 0; i < threads; ++i) {
            workers.push_back(std::make_shared<boost::thread>(&ThreadPool::work, this));
        }
    }
    catch (...) {
        stop = true;
        for (size_t i = 0; i < workers.size(); ++i) {
            if (workers[i]->joinable()) {
                workers[i]->join();
            }
        }
        throw;
    }
}

void ThreadPool::Wait() {
    if (!init) {
        throw std::exception("ThreadPool is not initialized");
    }

    boost::unique_lock<boost::mutex> lock(queue_mutex);
    wait = true;
    waitCount = 0;
    workCondition.notify_all();

    waitMainThreadCondition.wait(lock, [&]() { return waitCount == workers.size(); });
    wait = false;
    waitCondition.notify_all();
}

ThreadPool::~ThreadPool() {
    {
        boost::unique_lock<boost::mutex> lock(queue_mutex);
        stop = true;
    }

    workCondition.notify_all();
    for (size_t i = 0; i < workers.size(); ++i) {
        if (workers[i]->joinable()) {
            workers[i]->join();
        }
    }
}