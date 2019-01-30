#include "timer.h"

static const int SLEEP_DURATION = 100; // In milliseconds

Timer::Timer(int min_duration, int max_duration, function<void()> timer_callback) :
        min_duration(min_duration), max_duration(max_duration) {

    Reset();

    timer_thread = thread([this, timer_callback] () {
        while (!destroyed) {
            this_thread::sleep_for(chrono::milliseconds(SLEEP_DURATION));
            remaining_time -= SLEEP_DURATION;
            if (remaining_time <= 0) {
                timer_callback();
                Reset();
            }
        }
    });
}

Timer::~Timer() {
    destroyed = true;
    timer_thread.join();
}

void Timer::Reset() {
    remaining_time = min_duration + rand() % (max_duration - min_duration + 1);
    cout << "Resetting timer to " << remaining_time << "ms" << endl;
}
