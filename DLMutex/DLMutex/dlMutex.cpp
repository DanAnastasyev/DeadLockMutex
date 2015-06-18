#pragma once
#include <boost/thread.hpp>
#include <future>
#include <chrono>
#include <string>
#include "Synchronizer.h"
#include "dlMutex.h"

Synchronizer dlMutex::synch;

dlMutex::dlMutex() {
    boost::lock_guard<boost::mutex> lock(synch.mutex);
    id = ++synch.mutexsIdCount;
}

dlMutex::~dlMutex() {
}

void dlMutex::lock() {
    long long thisThreadId = synch.threadId(boost::this_thread::get_id());
    // Реализация оптимистичная: пока всё лочится без проблем - дедлоков нет
    if (!thisMutex.try_lock_for(boost::chrono::milliseconds(1000))) {
        synch.graph.addEdge(thisThreadId, id);
        try {
            // Запускаем проверку на циклы в отдельном потоке
            synch.taskQueue.AddTask(&Synchronizer::checkCycle);
            // Ждем, пока мьютекс таки залочится, либо проверка укажет на наличие дедлока
            while (!thisMutex.try_lock_for(boost::chrono::milliseconds(100))) {
                if (Synchronizer::isInterrupted(thisThreadId)) {
                    throw boost::thread_interrupted();
                }
                else {
                    synch.taskQueue.AddTask(&Synchronizer::checkCycle);
                }
            }
        }
        catch (boost::thread_interrupted&) {
            synch.graph.deleteEdge(thisThreadId, id);
            std::string msg = "Deadlock was found in mutex #" + boost::lexical_cast<std::string>(id)+'\n';
            throw std::logic_error(msg.c_str());
        }
        // Если попали сюда, проверять на циклы дальше не будем, прерываем тред
        synch.taskQueue.interrupt();
        synch.graph.deleteEdge(thisThreadId, id);
    }
    synch.graph.addEdge(id, thisThreadId);
}

void dlMutex::unlock() {
    long long thisThreadId = synch.threadId(boost::this_thread::get_id());
    thisMutex.unlock();
    synch.graph.deleteEdge(id, thisThreadId);
}

bool dlMutex::try_lock() {
    if (thisMutex.try_lock()) {
        synch.graph.addEdge(id, synch.threadId(boost::this_thread::get_id()));
        return true;
    }
    return false;
}