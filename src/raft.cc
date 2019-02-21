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
    args.RegisterString("config", "Path to configuration file (default = ./config)");
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
        storage.Reset();
    }

    RaftConfig raft_config(config_path);

    try {
        raft_config.parse(server_id);
    } catch (RaftConfigException& err) {
        error("%s", err.what());
        return EXIT_FAILURE;
    }

    vector<ServerInfo> server_infos = raft_config.get_server_infos();
    vector<PeerInfo> peer_infos = raft_config.get_peer_infos();

    RaftServer raft_server(server_id, server_infos, peer_infos);
    try {
        raft_server.Run();
    } catch (exception& err) {
        error("%s", err.what());
        return EXIT_FAILURE;
    }

    // Keep program alive until a SIGINT, SIGTERM, or SIGKILL is received
    sigset_t mask;
    sigemptyset(&mask);
    while (true) {
        sigsuspend(&mask);
    }

    google::protobuf::ShutdownProtobufLibrary();
    return EXIT_SUCCESS;
}
