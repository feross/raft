#pragma once

#include <string>
#include <unistd.h>

#include "log.h"
#include "subprocess.h"
#include "state-machine.h"
#include "util.h"

using namespace std;

static const int READ_BUFFER_SIZE = 1'000; // bytes

class BashStateMachine : public StateMachine {
    public:
        BashStateMachine() {}
        ~BashStateMachine() {}
        string Apply(string command);
    private:
};
