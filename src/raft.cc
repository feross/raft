#include <ctime>
#include <exception>
#include <iostream>
#include <unistd.h>

#include "arguments.h"
#include "raft-server.h"
#include "storage.h"

using namespace std;

static const string STORAGE_NAME_SUFFIX = "storage.dat";

static const string INTRO_TEXT =
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

    // Seed the pseudo-random number generator used by rand()
    srand(time(0));

    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterInt("port", "Listening port");
    args.RegisterString("id", "Server identifier");
    args.RegisterBool("reset", "Reset storage file");

    try {
        args.Parse(argc, argv);
    } catch (exception& err) {
        cerr << "Error: " << err.what() << endl;
        return EXIT_FAILURE;
    }

    string server_id = args.get_string("id");
    int port = args.get_int("port");

    if (args.get_bool("help")) {
        cout << args.get_help_text() << endl;
        return EXIT_SUCCESS;
    }

    // TODO: Remove once Arguments supports required args
    if (server_id.size() == 0) {
        cerr << "Server id is required" << endl;
        return EXIT_FAILURE;
    }

    Storage storage(server_id + "-" + STORAGE_NAME_SUFFIX);
    storage.set_current_term(0);
    storage.set_voted_for("");

    if (args.get_bool("reset")) {
        storage.Reset();
        return EXIT_SUCCESS;
    }

    // TODO: Remove once Arguments supports required args
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

    struct PeerInfo peer_info;
    peer_info.my_listen_port = port;
    peer_info.destination_port = connect_port;
    peer_info.destination_ip_addr = "127.0.0.1";
    vector<struct PeerInfo> peer_info_vector;
    peer_info_vector.push_back(peer_info);

    RaftServer raft_server(server_id, storage, peer_info_vector);

    // TODO: hack for now, b/c we're on localhost & no other way to distinguish connections
    assert(port != connect_port);

    while (true) {
        sleep(10);
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}
