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
        StateMachine() {}
        virtual ~StateMachine() {}
        virtual string Apply(string command) = 0;
};
