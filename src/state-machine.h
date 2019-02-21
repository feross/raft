/**
 * This class represents the Raft concept of a state machine in a generic way.
 * This is an abstract class and it should be subclassed to create a specific
 * type of state machine. This interface is the most minimal interface for a
 * state machine. There's a single `Apply` method to apply state transitions to
 * the state machine.
 *
 * For this Raft project, we create a single subclass called `BashStateMachine`
 * which is defined in bash-state-machine.h.
 */

#pragma once

#include <string>

using namespace std;

class StateMachineException : public exception {
    public:
        StateMachineException(const string& message): message(message) {}
        StateMachineException(const char* message): message(message) {}
        const char* what() const noexcept { return message.c_str(); }
    private:
        string message;
};

class StateMachine {
    public:
        /**
         * Creates a state machine instance. Can store state internally as
         * private instance variables or interact with an external system like
         * a terminal or remote database. Should be overriden in the subclass.
         */
        StateMachine() {}

        /**
         * Destroy the state machine. Should be overriden in the subclass.
         */
        virtual ~StateMachine() {}

        /**
         * Apply a command (or state transition) to the state machine. Should be
         * overriden in the subclass.
         *
         * @param  command Command that describes the state transition
         * @return State after the state transition
         */
        virtual string Apply(string command) = 0;
};
