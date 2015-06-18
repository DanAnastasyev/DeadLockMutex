#pragma once
#include "Synchronizer.h"

// Класс-обертка над boost::thread
// Нужен для отслеживания всех имеющихся потоков и возможности кинуть исключение в поток с deadlock 
class dlThread : public boost::thread {
public:
    dlThread();
    ~dlThread();

    template <class Fn, class... Args>
    explicit dlThread(Fn&& fn, Args&&... args);

    dlThread(const dlThread&) = delete;
    thread& operator= (const thread&) = delete;

    boost::thread::id get_id() const;

    bool joinable() const;
    void join();
private:
    boost::thread* thisThread;
    Synchronizer synch;
};

template <class Fn, class... Args>
dlThread::dlThread(Fn&& fn, Args&&... args) {
    thisThread = Synchronizer::addDlThread(fn, args...);
}