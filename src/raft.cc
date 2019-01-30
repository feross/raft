#include <exception>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "arguments.h"
#include "peer.h"
#include "peermessage.pb.h"
#include "storage.h"
#include "timer.h"

using namespace std;

static const string STORAGE_PATH = "storage.dat";

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

void send(Peer* peer, proto::PeerMessage &message) {
    string message_string;
    message.SerializeToString(&message_string);
    const char* message_cstr = message_string.c_str();
    peer->SendMessage(message_cstr, message_string.size());
}

int main(int argc, char* argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterInt("port", "Listening port");
    args.RegisterString("name", "Server name");
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

    Storage storage(STORAGE_PATH);

    if (args.get_bool("reset")) {
        storage.Reset();
        return EXIT_SUCCESS;
    }

    string server_name = args.get_string("name");
    if (server_name.size() == 0) {
        cerr << "Server name is required" << endl;
        return EXIT_FAILURE;
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

    // Start the server

    int term = 0;

    // Create a peer
    const char* dest_addr = "127.0.0.1";
    Peer* associate = new Peer(port, dest_addr, connect_port,
        [](char* message, int message_len) -> void {
            cout << "Received message of length " << message_len << endl;
            proto::PeerMessage peer_message;
            peer_message.ParseFromString(string(message, message_len));
            printf("message received: %s\n", peer_message.DebugString().c_str());
        });

    Timer timer(5'000, 10'000, [&]() -> void {
        // Start an election!
        term += 1;
        proto::PeerMessage message;
        message.set_type(proto::PeerMessage::REQUESTVOTE_REQUEST);
        message.set_term(term);
        message.set_sender_id(server_name);
        send(associate, message);
    });

    // while (true) {
    //     proto::PeerMessage peer_message;
    //     peer_message.set_type(proto::PeerMessage::APPENDENTRIES_REQUEST);
    //     peer_message.set_term(999);
    //     peer_message.set_sender_id(server_name);
    //     cout << "Sending " << peer_message.DebugString() << endl;

    //     send(associate, peer_message);
    //     sleep(1);
    // }
    // delete(associate);

    while (true) {
        sleep(10);
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}
