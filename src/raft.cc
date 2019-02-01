#include "raft.h"

using namespace std;

LogType LOG_LEVEL = INFO;

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
    args.RegisterBool("debug", "Show debug logs");
    args.RegisterBool("quiet", "Show only warnings and errors");

    try {
        args.Parse(argc, argv);
    } catch (exception& err) {
        LOG(ERROR) << err.what();
        return EXIT_FAILURE;
    }

    if (args.get_bool("help")) {
        LOG() << args.get_help_text();
        return EXIT_SUCCESS;
    }

    if (args.get_bool("quiet")) {
        LOG_LEVEL = WARN;
    } else if (args.get_bool("debug")) {
        LOG_LEVEL = DEBUG;
    }

    string server_id = args.get_string("id");
    if (server_id.size() == 0) {
        LOG(ERROR) << "Server identifier is required (use --id)";
        return EXIT_FAILURE;
    }

    if (args.get_bool("reset")) {
        Storage storage(server_id + STORAGE_NAME_SUFFIX);
        storage.Reset();
        return EXIT_SUCCESS;
    }

    vector<string> peer_info_strs = args.get_unnamed();
    if (peer_info_strs.size() == 0) {
        LOG(ERROR) << "Specify at least one peer to connect to";
        return EXIT_FAILURE;
    }

    vector<struct PeerInfo> peer_infos;
    for (string peer_info_str: peer_info_strs) {
        auto parts = Util::StringSplit(peer_info_str, ":");
        struct PeerInfo peer_info;
        peer_info.destination_ip_addr = parts[0];
        peer_info.my_listen_port = stoi(parts[1]);
        peer_info.destination_port = stoi(parts[2]);
        peer_infos.push_back(peer_info);
    }

    RaftServer raft_server(server_id, peer_infos);

    // Keep the main thread alive until a SIGINT or SIGTERM is received
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    google::protobuf::ShutdownProtobufLibrary();

    return EXIT_SUCCESS;
}
