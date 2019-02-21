#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "raft-config.h"
#include "thread-pool.h"
#include "util.h"

using namespace std;

const static int THREAD_POOL_SIZE = 8;

typedef function<int(char * command)> RequestCallback;

class ClientServerException : public exception {
    public:
        ClientServerException(const string& message): message(message) {}
        ClientServerException(const char* message): message(message) {}
        const char* what() const noexcept { return message.c_str(); }
    private:
        string message;
};

class ClientServer {
    public:
        ClientServer(RequestCallback request_callback);
        ~ClientServer();
        void Listen(unsigned short listen_port);
        void RespondToClient(int request_id, string& response);
        void RedirectToServer(ServerInfo *server_info);

    private:
        void HandleClientConnection(int client_socket);
        RequestCallback request_callback;
        ThreadPool thread_pool;
        ServerInfo * redirect_server_info;
        map<int, int> pending_client_sockets; // request_id -> client_socket
};
