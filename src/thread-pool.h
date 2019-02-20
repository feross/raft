// From CS110 implementation

/**
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <thread>
#include <vector>

#include "semaphore.h"

using namespace std;

class ThreadPool {
    public:

        /**
         * Constructs a ThreadPool configured to spawn up to the specified
         * number of threads.
         */
        ThreadPool(size_t numThreads);

        /**
         * Schedules the provided thunk (which is something that can
         * be invoked as a zero-argument function without a return value)
         * to be executed by one of the ThreadPool's threads as soon as
         * all previously scheduled thunks have been handled.
         */
        void schedule(const std::function<void(void)>& thunk);

        /**
         * Blocks and waits until all previously scheduled thunks
         * have been executed in full.
         */
        void wait();

        /**
         * Waits for all previously scheduled thunks to execute, and then
         * properly brings down the ThreadPool and any resources tapped
         * over the course of its lifetime.
         */
        ~ThreadPool();

    private:
        void dispatcher();
        void worker(size_t workerID);
        size_t getAvailableWorker();

        struct context {
            context(): isRunning(false), isAvailable(true) {}
            bool isRunning;
            bool isAvailable;
            semaphore dataIsReady;
            std::function<void(void)> thunk;
            std::thread thread;
        };

        std::thread dispatcherThread;
        bool poolIsRunning;

        semaphore numWaiting;
        semaphore numAvailableWorkers;
        std::vector<context> contexts;
        std::mutex readyListLock;
        std::deque<std::function<void(void)>> readyList;

        int outstandingThunkCount;
        std::mutex outstandingThunkCountLock;
        std::condition_variable_any allDone;

        ThreadPool(const ThreadPool& original) = delete;
        ThreadPool& operator=(const ThreadPool& rhs) = delete;
};
