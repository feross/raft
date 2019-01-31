#pragma once

#include <iostream>

#include "ostreamlock.h"

using namespace std;

enum LogType {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

struct LogConfig {
    bool headers = false;
    LogType level = WARN;
};

extern LogConfig LOGCFG;

class LOG {
    public:
        LOG(LogType level);
        ~LOG();

        template<class T>
        LOG& operator<<(const T &msg) {
            if (level >= LOGCFG.level) {
                if (level == ERROR) {
                    cerr << msg;
                } else {
                    cout << msg;
                }
                opened = true;
            }
            return *this;
        }
    private:
        bool opened = false;
        LogType level = DEBUG;
        inline string getLabel(LogType type);
};
