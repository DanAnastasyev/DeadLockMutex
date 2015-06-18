#pragma once
#include "ThreadPool.h"
#include "ThreadQueue.h"

ThreadQueue::ThreadQueue() : ThreadPool(1) {}

void ThreadQueue::interrupt() {
    ThreadPool::workers[0]->interrupt();
}