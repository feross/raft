#pragma once

#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "arguments.h"
#include "log.h"
#include "raft-config.h"

using namespace std;
using namespace chrono;

/**
 * The number of times to retry a client request before treating it as failure.
 */
const static int MAX_CLIENT_RETRIES = 30;

/**
 * The time to wait after a failed client request.
 */
const static duration CLIENT_RETRY_DELAY = seconds(3);

/**
 * Vector of information about the servers in the cluster.
 */
static vector<ServerInfo> server_infos;

/**
 * Server information for the server that is leader of the Raft cluster.
 */
static ServerInfo leader_server_info;

/**
 * Help text for the ./client command line program.
 */
static const string INTRO_TEXT =
R"(Raft Client

Usage:
    ./client [options]

    The purpose of Raft is to make a bunch of servers work together to reliably
    update a state machine. This program allows you to actually update the state
    in the state machine.

    The `./client` program creates a REPL that sends commands to the Raft
    cluster. The client automatically handles finding the leader server,
    retrying the request with a different server if the leader becomes
    unavailable, and displaying the output from running each command.

    Here's what a sample run of the client looks like:
        $ ./client
        > echo hello
        hello
        > touch myfile.txt
        > ls
        myfile.txt
        > quit
)";

/**
 * Send the given `command` to the leader of the Raft cluster. If the request
 * fails for any reason it will be retried for a set number of times. If the
 * contacted server redirects us to another server, the client closes the
 * connection and connects to the new server.
 *
 * Blocks until a successful response is received from a server, or the request
 * runs out of retries and fails.

 * @param  command User-provided command that should be sent to the leader of
 *     the Raft cluster.
 * @return Whether the command succeeded or not.
 */
bool send_command(const char * command);
