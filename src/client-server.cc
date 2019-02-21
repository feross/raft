#include "client-server.h"

ClientServer::ClientServer(RequestCallback request_callback) :
    request_callback(request_callback), thread_pool(THREAD_POOL_SIZE) {
}

ClientServer::~ClientServer() {
    thread_pool.wait();
}

void ClientServer::Listen(unsigned short listen_port) {
    // Populate server information struct
    struct sockaddr_in server_info;
    memset(&server_info, 0, sizeof(struct sockaddr_in));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = htonl(INADDR_ANY); // Accept on any interface
    server_info.sin_port = htons(listen_port);

    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        throw ClientServerException("Error creating server socket");
    }

    // Set socket options
    int val = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
        throw ClientServerException("Error setting socket options");
    }

    // Bind socket to address
    if (::bind(server_socket, (struct sockaddr *) &server_info, sizeof(struct sockaddr)) == -1) {
        throw ClientServerException("Error binding socket to address: " + to_string(listen_port));
    }

    // Start listening, with queue of up to 1 connection
    if (listen(server_socket, 1) == -1) {
        throw ClientServerException("Error listening to socket");
    }

    info("Client server started on port %d", listen_port);

    while (true) {
        struct sockaddr_in client_info;
        socklen_t size = sizeof(struct sockaddr_in);
        int client_socket = accept(server_socket, (struct sockaddr *) &client_info, &size);

        if (client_socket == -1) {
            warn("Error accepting socket: %s", strerror(errno));
            continue;
        }
        debug("Client connection from %s:%d", inet_ntoa(client_info.sin_addr),
            ntohs(client_info.sin_port));

        thread_pool.schedule([this, client_socket]() {
            HandleClientConnection(client_socket);
        });
    }

    if (close(server_socket) == -1) {
        warn("Error closing socket %d: %s", server_socket, strerror(errno));
    }
}

void ClientServer::RespondToClient(int request_id, string& response) {
    int client_socket = pending_client_sockets[request_id];
    debug("Responding to client (request_id: %d, response: %s)", request_id, response.c_str());
    const char * response_cstr = response.c_str();

    // TODO: error check
    int len = strlen(response_cstr);
    write(client_socket, &len, sizeof(int));
    dprintf(client_socket, "%s", response_cstr);
    if (close(client_socket) == -1) {
        warn("Error closing socket %d (%s)", client_socket, strerror(errno));
    }
}

void ClientServer::RedirectToServer(ServerInfo * server_info) {
    // Close pending client sockets so clients attempt to find new leader
    for (pair<int, int> pending_client_socket: pending_client_sockets) {
        int client_socket = pending_client_socket.second;
        if (close(client_socket) == -1) {
            warn("Error closing socket %d (%s)", client_socket, strerror(errno));
        }
    }
    pending_client_sockets.clear();
    redirect_server_info = server_info;
}

void ClientServer::HandleClientConnection(int client_socket) {
    if (redirect_server_info != NULL) {
        int redirect_sentinel = -1;
        write(client_socket, &redirect_sentinel, sizeof(int));
        write(client_socket, redirect_server_info, sizeof(ServerInfo));
        return;
    }

    int message_size;
    int bytes_read = 0;
    while (bytes_read < sizeof(int)) {
        void * dest = (char *) &message_size + bytes_read;
        int new_bytes = recv(client_socket, dest, sizeof(int) - bytes_read, 0);
        if (new_bytes == -1) {
            warn("Error reading from socket %d (%s)", client_socket, strerror(errno));
            if (close(client_socket) == -1) {
                warn("Error closing socket %d (%s)", client_socket, strerror(errno));
            }
            return;
        }
        bytes_read += new_bytes;
    }

    char buf[message_size + 1];
    bytes_read = 0;
    while (bytes_read < message_size) {
        void * dest = buf + bytes_read;
        int new_bytes = recv(client_socket, dest, message_size - bytes_read, 0);
        if (new_bytes == 0 || new_bytes == -1) {
            warn("Error reading from socket %d (%s)", client_socket, strerror(errno));
            if (close(client_socket) == -1) {
                warn("Error closing socket %d (%s)", client_socket, strerror(errno));
            }
            return;
        }
        bytes_read += new_bytes;
    }
    // Read complete message from client
    buf[message_size] = '\0';
    int request_id = request_callback(buf);
    pending_client_sockets[request_id] = client_socket;
}
