#include <exception>
#include <iostream>

#include "arguments.h"

using namespace std;

static string INTRO_TEXT =
R"(Raft - An understandable consensus algorithm

Usage:
    ./raft [options] [peers ...]

Example:
    Start a server that is part of a three server Raft cluster.

        ./raft --port 4000 localhost:4001 localhost:4002

Start a Raft server that listens on the given *port*. The server will treat
each operand in *peers* as a peer Raft server part of the same cluster.
These arguments should be hostname:port pairs in the format e.g.
localhost:4000 or e.g. 12.34.56.67:4000.
)";

int main(int argc, char* argv[]) {
    Arguments args(INTRO_TEXT);
    args.register_bool("help", "Print help message");
    args.register_int("port", "Set listening port");
    args.register_bool("clear", "Clear stored state");

    try {
        args.parse(argc, argv);
    } catch (exception& err) {
        cerr << "Error: " << err.what() << endl;
        return EXIT_FAILURE;
    }

    if (args.get_bool("help")) {
        cout << args.get_help_text() << endl;
        return EXIT_SUCCESS;
    }

    if (args.get_bool("clear")) {
        cout << "TODO: Clear stored state!" << endl;
        return EXIT_SUCCESS;
    }

    int port = args.get_int("port");
    if (port == 0) {
        cerr << "Missing required --port argument" << endl;
        return EXIT_FAILURE;
    }
    cout << "Using port " << port << endl;

    return EXIT_SUCCESS;
}

