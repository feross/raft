#include "timer.h"

Timer::Timer(int min_duration, int max_duration, TimerCallback timer_callback) :
        min_duration(min_duration), max_duration(max_duration),
        timer_callback(timer_callback) {
    Reset();
    timer_thread = thread([this] () {
        RunTimerThread();
    });
}

Timer::~Timer() {
    destroyed = true;
    timer_cv.notify_all();
    timer_thread.join();
}

void Timer::Reset() {
    lock_guard<mutex> lock(timer_mutex);

    time_point<system_clock> now = system_clock::now();
    int remaining_time = min_duration + rand() % (max_duration - min_duration + 1);

    active = true;
    timeout_time = now + milliseconds(remaining_time);

    debug("Reset timer (%d)", remaining_time);
    timer_cv.notify_all();
}

void Timer::RunTimerThread() {
    timer_mutex.lock();

    while (true) {
        while (!destroyed && !active) {
            timer_cv.wait(timer_mutex);
        }
        while (!destroyed && system_clock::now() < timeout_time) {
            timer_cv.wait_until(timer_mutex, timeout_time);
        }
        if (destroyed) break;

        // Reset state before callback in case it calls Reset()
        active = false;
        timer_mutex.unlock();

        timer_callback();
    }
}
