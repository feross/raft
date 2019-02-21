#include "client.h"

using namespace std;

LogType LOG_LEVEL = INFO;

static ServerInfo server_info;

bool send_command(const char * command) {
    int retries = 10;
    while (retries > 0) {
        // Populate leader information struct
        struct sockaddr_in leader_info;
        memset(&leader_info, 0, sizeof(leader_info));
        leader_info.sin_family = AF_INET;
        leader_info.sin_addr.s_addr = inet_addr(server_info.ip_addr.c_str());
        leader_info.sin_port = htons(server_info.port);

        // Create socket
        int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (leader_socket == -1) {
            error("Error creating server socket (%s)", strerror(errno));
            retries--;
            continue;
        }

        // Connect socket
        if (connect(leader_socket, (struct sockaddr *) &leader_info,
                sizeof(struct sockaddr_in)) == -1) {
            error("Error connecting to server (%s)", strerror(errno));
            retries--;
            continue;
        }

        struct sockaddr_in local_info;
        socklen_t size = sizeof(struct sockaddr_in);
        if (getsockname(leader_socket, (struct sockaddr *) &local_info, &size) == -1) {
            error("Error getting local socket info (%s)", strerror(errno));
            retries--;
            continue;
        }

        info("Connect to server %s:%d", inet_ntoa(local_info.sin_addr),
            ntohs(local_info.sin_port));

        // TODO: error check
        int len = strlen(command);
        write(leader_socket, &len, sizeof(int));
        dprintf(leader_socket, "%s", command);

        // Read response from leader server
        int message_size;
        int bytes_read = 0;
        while (bytes_read < sizeof(int)) {
            void * dest = (char *) &message_size + bytes_read;
            int new_bytes = recv(leader_socket, dest, sizeof(int) - bytes_read, 0);
            if (new_bytes == -1) {
                warn("Error reading from socket %d (%s)", leader_socket, strerror(errno));
                if (close(leader_socket) == -1) {
                    warn("Error closing socket %d (%s)", leader_socket, strerror(errno));
                }
                retries--;
                continue;
            }
            bytes_read += new_bytes;
        }

        int redirect_to_leader = false;
        if (message_size == -1) {
            // Server is redirecting us to the true leader
            redirect_to_leader = true;
            message_size = sizeof(ServerInfo);
        }

        char buf[message_size + 1];
        bytes_read = 0;
        while (bytes_read < message_size) {
            void * dest = buf + bytes_read;
            int new_bytes = recv(leader_socket, dest, message_size - bytes_read, 0);
            if (new_bytes == 0 || new_bytes == -1) {
                warn("Error reading from socket %d (%s)", leader_socket, strerror(errno));
                if (close(leader_socket) == -1) {
                    warn("Error closing socket %d (%s)", leader_socket, strerror(errno));
                }
                retries--;
                continue;
            }
            bytes_read += new_bytes;
        }

        if (redirect_to_leader) {
            server_info = *(ServerInfo *) buf;
            info("Redirecting to leader: %s:%d", server_info.ip_addr.c_str(), server_info.port);
            retries--;
            continue;
        }

        // Read complete message from server
        buf[message_size] = '\0';
        printf("%s", buf);
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    Arguments args(INTRO_TEXT);
    args.RegisterBool("help", "Print help message");
    args.RegisterString("config", "Path to configuration file (default = ./config)");
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

    string config_path = args.get_string("config");
    if (config_path == "") {
        config_path = DEFAULT_CONFIG_PATH;
    }

    RaftConfig raft_config(config_path);

    try {
        raft_config.parse(0);
    } catch (RaftConfigException& err) {
        error("%s", err.what());
        return EXIT_FAILURE;
    }

    vector<ServerInfo> server_infos = raft_config.get_server_infos();

    server_info = server_infos[0];

    while (true) {
        string command;
        printf("> ");
        getline(cin, command);
        Util::Trim(command);

        if (cin.eof()) {
            // End-of-file received; quit client
            break;
        }
        if (command == "exit" || command == "quit") {
            // User explicitly quit client
            break;
        }
        if (command.empty()) {
            // Ignore empty commands
            continue;
        }
        bool success = send_command(command.c_str());

        if (!success) {
            error("%s", "Failed to execute command on a Raft server");
            return EXIT_FAILURE;
        }
    }
}
