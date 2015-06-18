#pragma once
#include "Synchronizer.h"
#include "dlThread.h"

dlThread::dlThread() {
    thisThread = Synchronizer::addDlThread();
}

boost::thread::id dlThread::get_id() const {
    return thisThread->get_id();
}

bool dlThread::joinable() const {
    return thisThread->joinable();
}

void dlThread::join() {
    thisThread->join();
}

dlThread::~dlThread() {
    boost::unique_lock<boost::mutex> lock(synch.mutex);
    synch.allThreads.remove(thisThread);
    delete thisThread;
}