#pragma once

#include "log.h"

using namespace std;

class StateMachine {
    public:
        void commit(string command);
        void commit(vector<string> commands);
    private:
};
