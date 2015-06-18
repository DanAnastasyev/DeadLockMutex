#pragma once
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <future>
#include <vector>
#include <queue>
#include <memory>
#include <functional>

// Пул потоков
class ThreadPool {
public:
    ThreadPool();
	ThreadPool(size_t workersCount);
	~ThreadPool();

	template<class FuncType, class... Args>
	std::future<typename std::result_of<FuncType(Args...)>::type> AddTask(FuncType&& f, Args&&... args);

	void Wait();

	void Init(size_t workersCount);
private:
	friend class ThreadQueue;

	bool init;
	bool stop;
	bool wait;
	size_t waitCount;
	std::queue< std::function<void()> > tasks;
	std::vector< std::shared_ptr<boost::thread> > workers;
	boost::mutex queue_mutex;
	boost::condition_variable workCondition;
	boost::condition_variable waitCondition;
	boost::condition_variable waitMainThreadCondition;

	void work();
};

template<class FuncType, class... Args>
std::future<typename std::result_of<FuncType(Args...)>::type> 
	ThreadPool::AddTask(FuncType&& f, Args&&... args) 
{
	if (!init) {
		throw std::exception("ThreadPool is not initialized");
	}

	typedef typename std::result_of<FuncType(Args...)>::type resultType;

	auto task = std::make_shared< std::packaged_task<resultType()> >
		( std::bind( std::forward<FuncType>(f), std::forward<Args>(args)... ) );

	std::future<resultType> res = task->get_future();
	{
		boost::unique_lock<boost::mutex> lock(queue_mutex);
		tasks.push([task]() { (*task)(); });
	}

	workCondition.notify_one();
	return res;
}

// f() должна принимать один целочисленный аргумент, пробегающий от begin до end, и не зависеть от прочих значений f()
template<class FuncType>
void parallelFor(size_t begin, size_t end, FuncType&& f) {
	if (end - begin < 128) {
		for (size_t i = begin; i < end; ++i) {
			f(i);
		}
	}
	else {
		size_t threadsCount = boost::thread::hardware_concurrency();
		ThreadPool pool(threadsCount);
		size_t delta = (end - begin) / threadsCount;
		std::vector<std::future<void>> futures;
		for (size_t i = 0; i < threadsCount; ++i) {
			pool.AddTask([=] {
				size_t localbegin = begin + i*delta;
				size_t localend = (i == threadsCount - 1) ? end : localbegin + delta;
				for (size_t it = localbegin; it < localend; ++it) {
					f(it);
				}
			});
		}
		pool.Wait();
	}
}