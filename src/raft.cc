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
    args.RegisterInt("id", "Server identifier");
    args.RegisterInt("listen", "Client listen port");
    args.RegisterInt("config", "Path to configuration file (default = ./config)");
    args.RegisterBool("reset", "Delete server storage");
    args.RegisterBool("debug", "Show all logs");
    args.RegisterBool("quiet", "Show only errors");

    try {
        args.Parse(argc, argv);
    } catch (ArgumentsException& err) {
        error("%s", err.what());
        return EXIT_FAILURE;
    }

    if (args.get_bool("quiet")) {
        LOG_LEVEL = ERROR;
    } else if (args.get_bool("debug")) {
        LOG_LEVEL = DEBUG;
    }

    if (args.get_bool("help")) {
        printf("%s\n", args.get_help_text().c_str());
        return EXIT_SUCCESS;
    }

    int server_id = args.get_int("id");
    if (server_id == -1) {
        error("%s", "Server identifier is required (use --id)");
        return EXIT_FAILURE;
    }

    string config_path = args.get_string("config");
    if (config_path == "") {
        config_path = "./config";
    }

    if (args.get_bool("reset")) {
        RaftStorage storage(to_string(server_id) + STORAGE_NAME_SUFFIX);
        try {
            storage.Reset();
        } catch (RaftStorageException& err) {
            error("%s", err.what());
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    RaftConfig raft_config(config_path);

    try {
        raft_config.parse(server_id);
    } catch (RaftConfigException& err) {
        error("%s", err.what());
        return EXIT_FAILURE;
    }

    return 0;

    // vector<PeerInfo> peer_infos;
    // for (string peer_info_str: peer_info_strs) {
    //     auto parts = Util::StringSplit(peer_info_str, ":");
    //     PeerInfo peer_info;
    //     peer_info.destination_ip_addr = parts[0];
    //     peer_info.my_listen_port = stoi(parts[1]);
    //     peer_info.destination_port = stoi(parts[2]);
    //     peer_infos.push_back(peer_info);
    // }

    // int listen_port = args.get_int("listen");
    // RaftServer raft_server(server_id, peer_infos, listen_port);

    // // Keep program alive until a SIGINT, SIGTERM, or SIGKILL is received
    // sigset_t mask;
    // sigemptyset(&mask);
    // while (true) {
    //     sigsuspend(&mask);
    // }

    // google::protobuf::ShutdownProtobufLibrary();
    // return EXIT_SUCCESS;
}
