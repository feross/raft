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
        /**
         * A logger class designed to standardize logging across an entire
         * project, ensures that logging is thread-safe, and is routed to STDOUT
         * or STDERR as appropriate.
         *
         * Users of this logger class should set the desired logging level using
         * the extern LOG_LEVEL variable. All log messages are categorized into
         * one of four levels, so only the desired logs can be included when
         * running a program
         *
         * Log levels:
         *     - DEBUG (most verbose)
         *     - INFO
         *     - WARN
         *     - ERROR (least verbose)
         *
         * Example:
         *     LogType LOG_LEVEL = INFO; // Exclude logs at DEBUG level
         *
         *     int main(int argc, char* argv[]) {
         *         LOG(DEBUG) << "This is a debug message"; // Not printed!
         *         LOG(INFO) << "This is some useful information";
         *         LOG(ERROR) << "Unexpected error: " << error_message;
         *     }
         */
        LOG(LogType level);

        /**
         * Zero argument constructor logger. Has a few differences from the
         * normal logger:
         *
         *     - Always prints, no matter what the log level is set to
         *     - Prints a bare message without a log level prefix (i.e. no
         *       prefix of "[DEBUG]")
         */
        LOG();

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
