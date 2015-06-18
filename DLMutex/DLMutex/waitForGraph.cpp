#pragma once
#include "Synchronizer.h"
#include <vector>
#include <memory>
#include <map>
#include <stack>
#include "waitForGraph.h"

boost::mutex WaitForGraph::graphMutex;
std::map<long long, std::pair<std::vector<long long>, VertexStats>> WaitForGraph::data;

WaitForGraph::WaitForGraph() {}
WaitForGraph::~WaitForGraph() {}

void WaitForGraph::addEdge(long long from, long long to) {
    boost::unique_lock<boost::mutex> lock(graphMutex);
    data[from];
    boost::unique_lock<boost::mutex> lock2(data[from].second.mutex);
    data[from].first.push_back(to);
}

void WaitForGraph::deleteEdge(long long from, long long to) {
    boost::unique_lock<boost::mutex> lock(graphMutex);
    boost::unique_lock<boost::mutex> lock2(data[from].second.mutex);
    for (auto it = data[from].first.begin(); it != data[from].first.end(); ++it) {
        if (*it == to) {
            data[from].first.erase(it);
            if (data[from].first.empty()) {
                lock2.unlock();
                data.erase(from);
            }
            return;
        }
    }
}