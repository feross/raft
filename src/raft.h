#pragma once

#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>
#include <unistd.h>

#include "arguments.h"
#include "log.h"
#include "raft-server.h"
#include "storage.h"

static const string INTRO_TEXT =
R"(Raft - An understandable consensus algorithm

Usage:
    ./raft [options] [peers ...]

Minimal Example:
    Start a server that connects to one other server.

        ./raft --id <server_id> <ip_address>:<listen_port>:<destination_port>

        Tells this instance of raft to connect to *ip_address* by sending to
        port *destination_port*, and receiving on *listen_port*. *--id* is
        required to specify the server's name, which is used to maintain its
        persistent storage as well as to identify itself to other servers in the
        cluster.

Cluster Example:
    Start a three server Raft cluster.

        ./raft --id alice 127.0.0.1:4000:4010 127.0.0.1:4001:4020
        ./raft --id bob 127.0.0.1:4010:4000 127.0.0.1:4011:4021
        ./raft --id carol 127.0.0.1:4020:4001 127.0.0.1:4021:4011
)";
