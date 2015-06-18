#pragma once
#include "ThreadPool.h"

// Обертка над ThreadPool
class ThreadQueue : public ThreadPool {
public:
    ThreadQueue();
	void interrupt();
};

