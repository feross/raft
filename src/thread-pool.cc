// From CS110 implementation

#include "thread-pool.h"

ThreadPool::ThreadPool(size_t numThreads) : poolIsRunning(true),
    numAvailableWorkers(numThreads), contexts(numThreads), outstandingThunkCount(0) {

    dispatcherThread = thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    outstandingThunkCountLock.lock();
    outstandingThunkCount++;
    outstandingThunkCountLock.unlock();
    readyListLock.lock();
    readyList.push_back(thunk);
    readyListLock.unlock();
    numWaiting.signal(); // important this come after outstandingThunkCount has been incremented
}

void ThreadPool::dispatcher() {
    while (true) {
        numWaiting.wait();
        if (!poolIsRunning) break;
        numAvailableWorkers.wait();
        size_t workerID = getAvailableWorker();
        readyListLock.lock();
        contexts[workerID].thunk = readyList.front();
        readyList.pop_front();
        readyListLock.unlock();
        contexts[workerID].dataIsReady.signal();
    }
}

void ThreadPool::worker(size_t workerID) {
    while (true) {
        contexts[workerID].dataIsReady.wait();
        if (!poolIsRunning) break;
        contexts[workerID].thunk();
        contexts[workerID].isAvailable = true;
        numAvailableWorkers.signal();
        lock_guard<mutex> lg(outstandingThunkCountLock);
        outstandingThunkCount--;
        if (outstandingThunkCount == 0) allDone.notify_all();
    }
}

size_t ThreadPool::getAvailableWorker() {
    for (size_t workerID = 0; workerID < contexts.size(); workerID++) {
        if (contexts[workerID].isAvailable) {
            contexts[workerID].isAvailable = false;
            if (!contexts[workerID].isRunning) {
                // only extend thread pool if all previously spawned worker threads are busy
                contexts[workerID].thread = thread([this](size_t workerID) {
                    worker(workerID);
                }, workerID);
                contexts[workerID].isRunning = true;
            }
            return workerID;
        }
    }

    assert(false);
}

void ThreadPool::wait() {
    lock_guard<mutex> lg(outstandingThunkCountLock);
    allDone.wait(outstandingThunkCountLock, [this]{
        return outstandingThunkCount == 0;
    });
}

ThreadPool::~ThreadPool() {
    wait();
    poolIsRunning = false;

    numWaiting.signal();
    for (size_t i = 0; i < contexts.size(); i++) {
        if (contexts[i].isRunning)
            contexts[i].dataIsReady.signal();
    }

    dispatcherThread.join();
    for (size_t i = 0; i < contexts.size(); i++) {
        if (contexts[i].isRunning)
            contexts[i].thread.join();
    }
}
