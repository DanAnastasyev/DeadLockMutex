#pragma once
#include <boost/thread.hpp>
#include <future>
#include <chrono>
#include <string>
#include "Synchronizer.h"

// Класс мьютекса
// Хранит бустовую реализацию мьютекса, свой id и объект-синхронизатор
class dlMutex {
public:
	dlMutex();
	dlMutex(const dlMutex&) = delete;
    dlMutex& operator=(const dlMutex&) = delete;
	~dlMutex();

	void lock();
	void unlock();
	bool try_lock();
private:
	boost::mutex thisMutex;
	long long id;
	static Synchronizer synch;
};