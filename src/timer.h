#ifndef _RAFT_TIMER_H_
#define _RAFT_TIMER_H_

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace std;

class Timer {
    public:
        /**
         * Create a timer that calls the timer_callback function after a random
         * duration of time has passed. The random duration will always be a
         * value betweeen min_duration and max_duration.
         *
         * Internally, a thread is created to manage the timer, and the
         * timer_callback function will be called from this new thread.
         *
         * @param min_duration Minimum amount of time to wait (in milliseconds)
         * @param max_duration Maximum amount of time to wait (in milliseconds)
         * @param timer_callback The function to call when time is up
         */
        Timer(int min_duration, int max_duration, function<void()> timer_callback);

        /**
         * Destroy the timer and cleanup all resources.
         */
        ~Timer();

        /**
         * Cancel the previous timer and reset the timer to a new random time
         * (between min_duration and max_duration).
         */
        void Reset();
    private:
        int min_duration; // In milliseconds
        int max_duration; // In milliseconds
        int remaining_time; // In milliseconds
        thread timer_thread;
        bool destroyed = false;
};

#endif
