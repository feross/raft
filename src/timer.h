#ifndef _RAFT_TIMER_H_
#define _RAFT_TIMER_H_

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "ostreamlock.h"

using namespace std;

class Timer {
    public:
        /**
         * Create a timer that calls the callback function after a random
         * duration of time has passed. The random duration will always be a
         * value betweeen min_duration and max_duration.
         *
         * Internally, a thread is created to manage the timer, and the
         * callback function will be called from this new thread.
         *
         * Once the timer fires, the callback will not be called again until
         * Timer::Reset() is called to restart the timer.
         *
         * @param min_duration Minimum amount of time to wait (in milliseconds)
         * @param max_duration Maximum amount of time to wait (in milliseconds)
         * @param callback The function to call when the timer fires
         */
        Timer(int min_duration, int max_duration, function<void()> callback);
        Timer(int duration, function<void()> callback) : Timer(duration, duration, callback) {}

        /**
         * Destroy the timer and cleanup all resources.
         */
        ~Timer();

        /**
         * Cancel the previous timer (if it has not fired yet) and set the
         * remaining time to a new random time. The random duration will be
         * between min_duration and max_duration, as configured in the
         * constructor.
         *
         * Once the timer fires, call this method to restart the timer.
         */
        void Reset();
    private:
        int min_duration; // In milliseconds
        int max_duration; // In milliseconds

        int remaining_time; // In milliseconds
        thread timer_thread;
        bool destroyed = false;
        bool active = false;
};

#endif
