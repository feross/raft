// Inspired by the logger described here: https://stackoverflow.com/a/32262143

#include "log.h"

using namespace std;

LOG::LOG() : level(ERROR) {
    operator << (oslock);
}

LOG::LOG(LogType level) : level(level) {
    operator << (oslock);
    operator << ("[" + getLabel(level) + "] ");
}

LOG::~LOG() {   // what if we have 2 log objs in same block (so both are still in scope)
    if (opened) {
        if (level == ERROR) {
            cerr << endl;
        } else {
            cout << endl;
        }
        operator << (osunlock);
    }
    opened = false;
}

inline string LOG::getLabel(LogType type) {
    string label;
    switch (type) {
        case DEBUG:
            label = "DEBUG";
            break;
        case INFO:
            label = "INFO";
            break;
        case WARN:
            label = "WARN";
            break;
        case ERROR:
            label = "ERROR";
            break;
    }
    return label;
}
