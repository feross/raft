#pragma once

#include <iostream>

#include "ostreamlock.h"

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
                    std::cerr << msg;
                } else {
                    std::cout << msg;
                }
                opened = true;
            }
            return *this;
        }
    private:
        bool opened = false;
        LogType level = DEBUG;
        inline std::string getLabel(LogType type);
};
