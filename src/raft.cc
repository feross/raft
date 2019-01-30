#include <cstdlib>
#include <ctime>
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

void send(Peer *peer, proto::PeerMessage &message) {
    string message_string;
    message.SerializeToString(&message_string);
    const char* message_cstr = message_string.c_str();
    peer->SendMessage(message_cstr, message_string.size());
    // cout << "SENT\n" << message.DebugString() << endl;
}

int main(int argc, char* argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Seed the pseudo-random number generator used by rand()
    srand(time(0));

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

    string server_name = args.get_string("name");
    int port = args.get_int("port");

    if (args.get_bool("help")) {
        cout << args.get_help_text() << endl;
        return EXIT_SUCCESS;
    }

    // TODO: Remove once Arguments supports required args
    if (server_name.size() == 0) {
        cerr << "Server name is required" << endl;
        return EXIT_FAILURE;
    }

    Storage storage(server_name + "-" + STORAGE_NAME_SUFFIX);
    storage.set_current_term(0);
    storage.set_voted_for(0);

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

    cout << "Listening on " << port << ". Connecting to " << connect_port << "." << endl;

    // TODO: hack for now, b/c we're on localhost & no other way to distinguish connections
    assert(port != connect_port);

    Peer *peer;

    auto make_message = [&]() {
        proto::PeerMessage message;
        message.set_term(storage.current_term());
        message.set_sender_id(server_name);
        return message;
    };

    auto send_appendentries_request = [&](Peer *peer) {
        proto::PeerMessage message = make_message();
        message.set_type(proto::PeerMessage::APPENDENTRIES_REQUEST);
        send(peer, message);
    };

    auto send_appendentries_response = [&](Peer *peer, bool success) {
        proto::PeerMessage message = make_message();
        message.set_type(proto::PeerMessage::APPENDENTRIES_RESPONSE);
        message.set_success(success);
        send(peer, message);
    };

    auto send_requestvote_request = [&](Peer *peer) {
        proto::PeerMessage message = make_message();
        message.set_type(proto::PeerMessage::REQUESTVOTE_REQUEST);
        send(peer, message);
    };

    auto send_requestvote_response = [&](Peer *peer, bool vote_granted) {
        proto::PeerMessage message = make_message();
        message.set_type(proto::PeerMessage::REQUESTVOTE_RESPONSE);
        message.set_vote_granted(vote_granted);
        send(peer, message);
    };

    auto handleMessage = [&](char* raw_message, int raw_message_len) -> void {
        proto::PeerMessage message;
        message.ParseFromString(string(raw_message, raw_message_len));
        cout << "RECEIVED:\n" << message.DebugString() << endl;

        switch (message.type()) {
            case proto::PeerMessage::APPENDENTRIES_REQUEST:
                if (message.term() < storage.current_term()) {
                    send_appendentries_response(peer, true);
                }
                break;
            case proto::PeerMessage::APPENDENTRIES_RESPONSE:
                break;
            case proto::PeerMessage::REQUESTVOTE_REQUEST:
                if (message.term() < storage.current_term()) {
                    send_requestvote_response(peer, false);
                }
                // if (storage.)
                break;
            case proto::PeerMessage::REQUESTVOTE_RESPONSE:
                break;
            default:
                cerr << "Unexpected message: " << message.DebugString() << endl;
                break;
        }
    };

    peer = new Peer(port, "127.0.0.1", connect_port, handleMessage);

    auto handle_timeout = [&]() -> void {
        // Start an election!
        storage.set_current_term(storage.current_term() + 1);
        send_requestvote_request(peer);
    };

    Timer timer(5'000, 10'000, handle_timeout);

    while (true) {
        sleep(10);
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}
