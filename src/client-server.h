#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "thread-pool.h"
#include "util.h"

using namespace std;

const static int MAX_CLIENT_MESSAGE_SIZE = 1'000'000; // bytes
const static int THREAD_POOL_SIZE = 8;

typedef function<void(char * command)> RequestCallback;

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
        void HandleClientConnection(int client_socket);
    private:
        RequestCallback request_callback;
        ThreadPool thread_pool;
};
