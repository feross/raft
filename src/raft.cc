#include <exception>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "arguments.h"
#include "peer.h"
#include "storage.h"

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
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterInt("port", "Listening port");
    args.RegisterBool("reset", "Reset storage file");

    try {
        args.Parse(argc, argv);
    } catch (exception& err) {
        cerr << "Error: " << err.what() << endl;
        return EXIT_FAILURE;
    }

    if (args.get_bool("help")) {
        cout << args.get_help_text() << endl;
        return EXIT_SUCCESS;
    }

        cout << "TODO: Clear stored state!" << endl;
    if (args.get_bool("reset")) {
        return EXIT_SUCCESS;
    }

    int port = args.get_int("port");
    if (port == 0) {
        cerr << "Missing required --port argument" << endl;
        return EXIT_FAILURE;
    }
    vector<string> unnamed_args = args.get_unnamed();
    if (unnamed_args.size() == 0) {
        cerr << "Specify at least peer to connect to" << endl;
        return EXIT_FAILURE;
    }
    int connect_port = stoi(unnamed_args[0]);

    cout << "Listening on " << port << endl;
    cout << "Connecting to " << connect_port << endl;

    // TODO: hack for now, b/c we're on localhost & no other way to distinguish connections
    assert(port != connect_port);

    Storage storage;

    // Create a peer
    const char* dest_addr = "127.0.0.1";
    Peer* associate = new Peer(port, dest_addr, connect_port);

    const char* msg = "wow !    ";
    while (true) {
        associate->SendMessage(msg);
        sleep(1);
    }
    delete(associate);

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}

