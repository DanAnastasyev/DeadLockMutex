#pragma once
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <map>
#include <boost/lexical_cast.hpp>
#include "waitForGraph.h"
#include "ThreadQueue.h"

// Класс-синхронизатор
// Умеет запускать проверку на наличие циклов среди мьютексов
class Synchronizer {
public:
	Synchronizer();
	~Synchronizer();
	
	// Возвращает id потока
	static long long threadId(boost::thread::id threadId);
	// Запускает dfs для поиска циклов
	static void nonRecursiveDfs(long long startVertex);
	// Проверяет на наличие циклов
	static void checkCycle();
	// Проверяет, не сказали ли прервать поток
    static bool isInterrupted(long long);

 	static boost::thread* addDlThread();
 	template <class Fn, class... Args>
 	static boost::thread* addDlThread(Fn&& fn, Args&&... args);

private:
	friend class WaitForGraph;
	friend class dlMutex;
	friend class dlThread;

	static long long mutexsIdCount;
	static std::map<boost::thread::id, std::pair<long long, bool>> threadsId;
	static std::list<boost::thread*> allThreads;
	static long long threadsIdCount;
	static boost::mutex mutex;
	static WaitForGraph graph;
	static ThreadQueue taskQueue;

	static void throwInterruptException(long long id);
};

/*template <class Fn, class... Args>
boost::thread* Synchronizer::addDlThread(Fn&& fn, Args&&... args) {
    boost::unique_lock<boost::mutex> lock(mutex);
    allThreads.emplace_back(new boost::thread(fn, args...));
    return allThreads.back();
}*/