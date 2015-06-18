#pragma once
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <map>
#include <boost/lexical_cast.hpp>
#include "waitForGraph.h"
#include "ThreadQueue.h"
#include "Synchronizer.h"
#include <intsafe.h>

WaitForGraph Synchronizer::graph;
long long Synchronizer::mutexsIdCount;
std::map<boost::thread::id, std::pair<long long, bool>> Synchronizer::threadsId;
long long Synchronizer::threadsIdCount;
boost::mutex Synchronizer::mutex;
ThreadQueue Synchronizer::taskQueue;
std::list<boost::thread*> Synchronizer::allThreads;

Synchronizer::Synchronizer() {
    mutexsIdCount = 0;
    threadsIdCount = 0;
}

Synchronizer::~Synchronizer() {}

long long Synchronizer::threadId(boost::thread::id threadId) {
    boost::unique_lock<boost::mutex> lock(mutex);
    std::map<boost::thread::id, std::pair<long long, bool>>::iterator it;
    if ((it = threadsId.find(threadId)) == threadsId.end()) {
        threadsId[threadId].first = --threadsIdCount;
        threadsId[threadId].second = false;
        return threadsIdCount;
    }
    else {
        return it->second.first;
    }
}

bool Synchronizer::isInterrupted(long long id) {
    boost::unique_lock<boost::mutex> lock(mutex);
    for (auto it = threadsId.begin(); it != threadsId.end(); ++it) {
        if (it->second.first == id) {
            return it->second.second;
        }
    }
    return false;
}

void Synchronizer::throwInterruptException(long long id) {
    if (id > 0) {
        return;
    }
    boost::unique_lock<boost::mutex> lock(mutex);
    for (auto it = threadsId.begin(); it != threadsId.end(); ++it) {
        if (it->second.first == id) {
            it->second.second = true;
        }
    }
}

// Если мьютекс в потоке, запустившем проверку, сумеет залочиться - данный поток прерывается 
// (по-видимому, дедлока нет, или нам очень повезло)
// Иначе - проверка дорабатывает до конца
// Если находит цикл - кидает исключение
// Проверка - обычный dfs
// Желательно, чтобы несколько функций одновременно работать не могли
void Synchronizer::checkCycle() {
    try {
        boost::unique_lock<boost::mutex> lock(graph.graphMutex);
        for (auto it = graph.data.begin(); it != graph.data.end(); ++it) {
            it->second.second.color = C_White;
            it->second.second.parent = LONG64_MIN;
        }
        lock.unlock();
        boost::this_thread::interruption_point();
        lock.lock();
        for (auto it = graph.data.begin(); it != graph.data.end(); ++it) {
            switch (it->second.second.color) {
            case C_White:
                lock.unlock();
                nonRecursiveDfs(it->first);
                lock.lock();
                break;
            case C_Black:
                break;
            case C_Grey:
            default:
                throwInterruptException(it->first);
            }
        }
    } catch (boost::thread_interrupted&) {
    }
}

void Synchronizer::nonRecursiveDfs(long long startVertex) {
    std::stack<long long> ourStack;
    ourStack.push(startVertex);
    while (!ourStack.empty()) {
        boost::this_thread::interruption_point();
        long long currVertex = ourStack.top();
        if (graph.data.at(currVertex).second.color == C_White) {
            graph.data.at(currVertex).second.color = C_Grey;
            boost::unique_lock<boost::mutex> lock0(graph.graphMutex);
            boost::unique_lock<boost::mutex> lock(graph.data[currVertex].second.mutex);
            lock0.unlock();
            for (auto adjacentEdgesIter = graph.data.at(currVertex).first.begin();
                adjacentEdgesIter != graph.data.at(currVertex).first.end(); ++adjacentEdgesIter)
            {
                boost::this_thread::interruption_point();
                switch (graph.data.at(*adjacentEdgesIter).second.color) {
                case C_White:
                    ourStack.push(*adjacentEdgesIter);
                    graph.data.at(*adjacentEdgesIter).second.parent = currVertex;
                    break;
                case C_Grey:
                    for (; currVertex != *adjacentEdgesIter; currVertex = graph.data.at(currVertex).second.parent) {
                        if (currVertex < 0) {
                            throwInterruptException(currVertex);
                        }
                    }
                    if (currVertex < 0) {
                        throwInterruptException(currVertex);
                    }
                    return;
                case C_Black:
                    break;
                }
            }
        }
        if (ourStack.top() == currVertex) {
            ourStack.pop();
            graph.data.at(currVertex).second.color = C_Black;
        }
    }
}