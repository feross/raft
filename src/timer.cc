#include "timer.h"

static const int SLEEP_DURATION = 100; // In milliseconds

Timer::Timer(int min_duration, int max_duration, function<void()> callback) :
        min_duration(min_duration), max_duration(max_duration) {
    Reset();
    timer_thread = thread([this, callback] () {
        while (!destroyed) {
            this_thread::sleep_for(chrono::milliseconds(SLEEP_DURATION));
            if (active) {
                remaining_time -= SLEEP_DURATION;
                if (remaining_time <= 0) {
                    active = false;
                    callback();
                }
            }
        }
    });
}

Timer::~Timer() {
    destroyed = true;
    timer_thread.join();
}

void Timer::Reset() {
    active = true;
    remaining_time = min_duration + rand() % (max_duration - min_duration + 1);
    debug("Reset timer (%d)", remaining_time);
}
