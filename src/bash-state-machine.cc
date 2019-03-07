#include "bash-state-machine.h"

string BashStateMachine::Apply(string command) {
    vector<string> command_tokens = Util::StringSplit(command, " ");

    // Create NULL-terminated arguments array
    const char * argv[command_tokens.size() + 1];
    for (int i = 0; i < command_tokens.size(); i++) {
        argv[i] = command_tokens[i].c_str();
    }
    argv[command_tokens.size()] = NULL;
    subprocess_t process = subprocess(const_cast<char **>(argv), false, true);

    // Read stdout from the subprocess into a string that is returned
    string result;
    while (true) {
        char buffer[READ_BUFFER_SIZE];
        int bytes_read = read(process.ingestfd, &buffer, READ_BUFFER_SIZE);
        if (bytes_read == -1) {
            throw StateMachineException("Error reading from command stdout");
        }
        if (bytes_read == 0) {
            // End-of-file
            break;
        }
        result.append(buffer, bytes_read);
    }
    return result;
}
