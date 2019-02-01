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

extern LogType LOG_LEVEL;

class LOG {
    public:
        LOG(LogType level);
        ~LOG();

        template<class T>
        LOG& operator<<(const T &msg) {
            if (level >= LOG_LEVEL) {
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
