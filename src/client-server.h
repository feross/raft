/**
 * A server that accepts connections from clients, processes a single string
 * command from the client, calls a user-defined callback function, and holds
 * onto the connection until the user is ready to respond to the client. When
 * the user is ready to respond, the response is passed to the appropriate
 * client and the connection is finally closed.
 *
 * The server starts out in a "waiting mode" where it merely holds onto incoming
 * connections until the user is ready to process them. (In Raft, this is useful
 * at server startup where the server is a follower and does not know which
 * server is the leader.)
 *
 * When the user is ready to accept connections, the server can be put into
 * "serving mode" which unblocks the connections that were waiting and
 * immediately processes new connections by calling the user-defined callback
 * function.
 *
 * The server can also be put into a "redirect mode" where it responds to
 * clients with a special "sentinel" response that points them to another server
 * where they should retry their request. The connection is then closed.
 *
 * This class is thread-safe (its methods can safely be called from different
 * threads).
 */

#pragma once

#include <arpa/inet.h>
#include <condition_variable>
#include <cstring>
#include <map>
#include <mutex>
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

/**
 * The "modes" that the client server can be in.
 */
enum ClientServerState {
    Waiting,
    Serving,
    Redirecting
};

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
        /**
         * Create a server to handle requests from many clients in parallel.
         *
         * The provided `request_callback` function is called whenever a client
         * sends a request. The function is provided with the client's request
         * (as a string argument). The callback is expected to handle the
         * client's request asyncronously and return a "request id" integer that
         * will be used to return a response to the client at some point in the
         * future by calling `RespondToClient`. The connection to the client
         * will be held open until the user sends a response to the client
         * using the "request id" they provided when the request was received.
         *
         * Internally, many threads are created to manage the connections, and
         * the callback function can be called from any of these new threads.
         *
         * Server starts out in "waiting mode" until `StartServing` or
         * `StartRedirecting` is called.
         *
         * @param request_callback Function to call when a request is received
         * from a client. Expects to get a "request id" as return value.
         */
        ClientServer(RequestCallback request_callback);

        /**
         * Destroy the client server and cleanup resources.
         */
        ~ClientServer();

        /**
         * Start listening for connections on the given `listen_port`.
         * Connections will be accepted on all interfaces.
         *
         * @param listen_port The port to listen for connections on.
         */
        void Listen(unsigned short listen_port);

        /**
         * Respond to a client's request. The request to respond to is identified
         * by the given `request_id` parameter. It should match what was returned
         * by the `request_callback` function when the request was initially
         * received.
         *
         * @param request_id Unique identifier for the request.
         * @param response   String to send to the client as a response to their
         *     request.
         */
        void RespondToClient(int request_id, string& response);

        /**
         * Start serving client requests. Puts server into "serving mode".
         */
        void StartServing();

        /**
         * Start redirecting clients to the given server. Puts server into
         * "redirecting mode".
         *
         * @param new_redirect_server_info Server to redirect clients to.
         */
        void StartRedirecting(ServerInfo *new_redirect_server_info);

    private:
        /**
         * Process a client connection. This function runs in its own thread.
         *
         * @param client_socket The incoming client socket to process
         */
        void HandleClientConnection(int client_socket);

        /**
         * The state of the client server. Starts out in "waiting mode" and
         * changes whenever `StartServing` or `StartRedirecting` is called.
         */
        ClientServerState server_state;

        /**
         * User-provided function to call whenever a client sends a request.
         */
        RequestCallback request_callback;

        /**
         * Pool of threads that will be used to process many client requests
         * in parallel.
         */
        ThreadPool thread_pool;

        /**
         * Contact information for the server that we will redirect to. Should
         * be set to NULL when the server is not in "redirecting mode".
         */
        ServerInfo * redirect_server_info = NULL;

        /**
         * Client sockets that are waiting for a response. Maps the "request id"
         * returned by the callback function to the socket file descriptor.
         */
        map<int, int> pending_client_sockets;

        /**
         * Synchronization primatives. The condition variable is used to make
         * the server hold connections until the server leaves "waiting mode".
         */
        condition_variable_any server_cv;
        mutex server_mutex;
};
