#include "client.h"

using namespace std;

LogType LOG_LEVEL = INFO;

/**
 * Peer connection information. Describes a peer that this server should connect
 * to. Peers are specified via destination_ip_addr and destination_port. The
 * port on which this server will listen for a incoming connection from this
 * peer is specified as my_listen_port.
 */
struct ServerInfo {
    string ip_addr;
    unsigned short port;
};

int main(int argc, char* argv[]) {
    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterBool("debug", "Show all logs");
    args.RegisterBool("quiet", "Show only errors");

    try {
        args.Parse(argc, argv);
    } catch (exception& err) {
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

    vector<string> server_info_strs = args.get_unnamed();
    if (server_info_strs.size() == 0) {
        error("%s", "Specify at least one server to connect to");
        return EXIT_FAILURE;
    }

    vector<struct ServerInfo> server_infos;
    for (string server_info_str: server_info_strs) {
        auto parts = Util::StringSplit(server_info_str, ":");
        struct ServerInfo peer_info;
        peer_info.ip_addr = parts[0];
        peer_info.port = stoi(parts[1]);
        server_infos.push_back(peer_info);
    }

    // Populate leader information struct
    struct sockaddr_in leader_info;
    memset(&leader_info, 0, sizeof(leader_info));
    leader_info.sin_family = AF_INET;
    leader_info.sin_addr.s_addr = inet_addr(server_infos[0].ip_addr.c_str());
    leader_info.sin_port = htons(server_infos[0].port);

    // Create socket
    int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (leader_socket == -1) {
        error("Error creating server socket (%s)", strerror(errno));
        return EXIT_FAILURE;
    }

    // Connect socket
    if (connect(leader_socket, (struct sockaddr *) &leader_info,
            sizeof(struct sockaddr_in)) == -1) {
        error("Error connecting to server (%s)", strerror(errno));
        return EXIT_FAILURE;
    }

    struct sockaddr_in local_info;
    socklen_t size = sizeof(struct sockaddr_in);
    if (getsockname(leader_socket, (struct sockaddr *) &local_info, &size) == -1) {
        error("Error getting local socket info (%s)", strerror(errno));
        return EXIT_FAILURE;
    }

    info("Connect to server %s:%d", inet_ntoa(local_info.sin_addr),
        ntohs(local_info.sin_port));

    const char * message = "Hello server!";
    int len = strlen(message);
    // TODO: error check
    write(leader_socket, &len, sizeof(int));
    dprintf(leader_socket, "%s", message);

    while (true);
}
