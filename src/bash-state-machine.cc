#include "bash-state-machine.h"

string BashStateMachine::Apply(string command) {
    vector<string> command_tokens = Util::StringSplit(command, " ");

    const char * argv[command_tokens.size() + 1];
    for (int i = 0; i < command_tokens.size(); i++) {
        argv[i] = command_tokens[i].c_str();
    }
    argv[command_tokens.size()] = NULL;

    subprocess_t process = subprocess(const_cast<char **>(argv), false, true);

    string result;
    char buf[READ_BUFFER_SIZE];
    while (true) {
        int bytes_read = read(process.ingestfd, &buf, READ_BUFFER_SIZE);
        if (bytes_read == -1) {
            throw StateMachineException("Error reading from command stdout");
        }
        if (bytes_read == 0) {
            // End-of-file
            return result;
        }
        result.append(buf, bytes_read);
    }
}
