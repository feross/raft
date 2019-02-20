/**
 * Timer class that calls a callback function after a certain amount of time has
 * passed.
 */

#pragma once

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <condition_variable>

#include "log.h"

using namespace std;
using namespace std::chrono;

typedef function<void()> TimerCallback;

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
         * @param timer_callback The function to call when the timer fires
         */
        Timer(int min_duration, int max_duration, TimerCallback timer_callback);
        Timer(int duration, TimerCallback timer_callback) :
            Timer(duration, duration, timer_callback) {}

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
        /**
         * Main body of the timer thread.
         */
        void RunTimerThread();

        /**
         * Minimum amount of time to wait (in milliseconds)
         */
        int min_duration;

        /**
         * Maximum amount of time to wait (in milliseconds)
         */
        int max_duration;

        /**
         * The function to call when the timer fires
         */
        TimerCallback timer_callback;

        /**
         * The absolute time when the callback should run.
         */
        time_point<system_clock> timeout_time;

        /**
         * Is the timer destroyed? Used to signal to the timer thread that it is
         * time to terminate.
         */
        bool destroyed = false;

        /**
         * Is the timer active? Used to signal to the timer thread that there
         * is a pending callback.
         */
        bool active = false;

        thread timer_thread;
        condition_variable_any timer_cv;
        mutex timer_mutex;
};
