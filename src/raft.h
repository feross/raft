#pragma once

#include <chrono>
#include <ctime>
#include <exception>
#include <signal.h>

#include "arguments.h"
#include "log.h"
#include "raft-config.h"
#include "raft-server.h"
#include "raft-storage.h"

/**
 * Help text for the ./raft command line program.
 */
static const string INTRO_TEXT =
R"(Raft - An understandable consensus algorithm

Usage:
    ./raft [options] [peers ...]

Minimal Example:
    Start a server that connects to one other server.

        ./raft --id <server_id> --reset

Cluster Example:
    Start a three server Raft cluster.

    Use the config file:
        127.0.0.1 4000 4001 4002
        127.0.0.1 5000 5001 5002
        127.0.0.1 6000 6001 6002

    Run:
    ./raft --id 0 --reset
    ./raft --id 1 --reset
    ./raft --id 2 --reset
)";
