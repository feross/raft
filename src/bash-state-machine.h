/**
 * This class takes terminal commands and runs them in `bash`, returning the
 * output as a string. The class conforms to the `StateMachine` interface
 * defined in `state-machine.h`, which is the interface that `RaftServer`
 * expects.
 */

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
        /**
         * Create a new bash state machine. There's no state stored internally
         * in this class. It merely "passes through" the commands that are
         * applied to it by running them in `bash`.
         */
        BashStateMachine() {}

        /**
         * Destroy the bash state machine.
         */
        ~BashStateMachine() {}

        /**
         * Apply a command to the bash state machine. The command is a terminal
         * command like "ls -lha", "echo hello", or "mkdir new_folder". Blocks
         * until the command is finished running.
         *
         * @param  command terminal command with arguments separated by spaces
         * @return Output of running the given terminal command in bash
         */
        string Apply(string command);
    private:
};
