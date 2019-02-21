#include "client.h"

LogType LOG_LEVEL = INFO;

int main(int argc, char* argv[]) {
    // Seed the pseudo-random number generator used by rand()
    srand(time(0));

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

    server_infos = raft_config.get_server_infos();
    leader_server_info = server_infos[rand() % server_infos.size()];

    // Start a REPL loop that processes the user's commands
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
            // User explicitly quit the client
            break;
        }
        if (command.empty()) {
            // Ignore empty commands
            continue;
        }
        bool success = send_command(command.c_str());

        if (!success) {
            error("%s", "Failed to execute command (retried too many times)");
            return EXIT_FAILURE;
        }
    }
}

bool send_command(const char * command) {
    int retries = MAX_CLIENT_RETRIES;
    send_loop: while (retries > 0) {
        info("Attempting to send command (%d retries left)", retries);
        if (retries < MAX_CLIENT_RETRIES) {
            this_thread::sleep_for(CLIENT_RETRY_DELAY);
            leader_server_info = server_infos[rand() % server_infos.size()];
        }
        retries--;

        // Populate leader information struct
        struct sockaddr_in leader_info;
        memset(&leader_info, 0, sizeof(leader_info));
        leader_info.sin_family = AF_INET;
        leader_info.sin_addr.s_addr = inet_addr(leader_server_info.ip_addr.c_str());
        leader_info.sin_port = htons(leader_server_info.port);

        // Create socket
        int leader_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (leader_socket == -1) {
            error("Error creating server socket (%s)", strerror(errno));
            Util::SafeClose(leader_socket);
            continue;
        }

        // Connect socket
        if (connect(leader_socket, (struct sockaddr *) &leader_info,
                sizeof(struct sockaddr_in)) == -1) {
            error("Error connecting to server (%s)", strerror(errno));
            Util::SafeClose(leader_socket);
            continue;
        }

        // Get information about the local socket
        struct sockaddr_in local_info;
        socklen_t size = sizeof(struct sockaddr_in);
        if (getsockname(leader_socket, (struct sockaddr *) &local_info, &size) == -1) {
            error("Error getting local socket info (%s)", strerror(errno));
            Util::SafeClose(leader_socket);
            continue;
        }
        info("Connect to server %s:%d (from %s:%d)",
            leader_server_info.ip_addr.c_str(), leader_server_info.port,
            inet_ntoa(local_info.sin_addr), ntohs(local_info.sin_port));

        int len = strlen(command);
        if (write(leader_socket, &len, sizeof(int)) == -1) {
            error("Could not write to socket %d (%s)", leader_socket, strerror(errno));
            return false;
        }
        dprintf(leader_socket, "%s", command);

        // Read response from leader server
        int message_size;
        int bytes_read = 0;
        while (bytes_read < sizeof(int)) {
            void * dest = (char *) &message_size + bytes_read;
            int new_bytes = recv(leader_socket, dest, sizeof(int) - bytes_read, 0);
            if (new_bytes == 0 || new_bytes == -1) {
                warn("Error reading from socket %d (%s)", leader_socket, strerror(errno));
                Util::SafeClose(leader_socket);
                goto send_loop;
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
                Util::SafeClose(leader_socket);
                goto send_loop;
            }
            bytes_read += new_bytes;
        }

        if (redirect_to_leader) {
            leader_server_info = *(ServerInfo *) buf;
            info("Redirecting to leader: %s:%d",
                leader_server_info.ip_addr.c_str(), leader_server_info.port);
            Util::SafeClose(leader_socket);
            continue;
        }

        // Finished reading complete message from server
        Util::SafeClose(leader_socket);
        buf[message_size] = '\0';
        printf("%s", buf);

        return true;
    }
    return false;
}
