#include "raft.h"

using namespace std;

int main(int argc, char* argv[]) {
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Seed the pseudo-random number generator used by rand()
    srand(time(0));

    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterString("id", "Server identifier");
    args.RegisterBool("reset", "Delete server storage");

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

    string server_id = args.get_string("id");

    // TODO: Remove once Arguments supports required args
    if (server_id.size() == 0) {
        cerr << "Server id is required" << endl;
        return EXIT_FAILURE;
    }

    Storage storage(server_id + STORAGE_NAME_SUFFIX);
    storage.set_current_term(0);
    storage.set_voted_for("");

    if (args.get_bool("reset")) {
        storage.Reset();
        return EXIT_SUCCESS;
    }

    vector<string> unnamed_args = args.get_unnamed();
    if (unnamed_args.size() == 0) {
        cerr << "Specify at least peer to connect to" << endl;
        return EXIT_FAILURE;
    }

    vector<struct PeerInfo> peer_info_vector;

    for (string arg: unnamed_args) {
        auto vec = Util::StringSplit(arg, ":");
        struct PeerInfo peer_info;
        peer_info.destination_ip_addr = vec[0];
        peer_info.my_listen_port = stoi(vec[1]);
        peer_info.destination_port = stoi(vec[2]);
        peer_info_vector.push_back(peer_info);
    }

    RaftServer raft_server(server_id, storage, peer_info_vector);

    while (true) {
        sleep(10);
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}
