#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>
#include <unistd.h>

#include "arguments.h"
#include "raft-server.h"
#include "storage.h"

static const string STORAGE_NAME_SUFFIX = "-storage.dat";

static const string INTRO_TEXT =
R"(Raft - An understandable consensus algorithm

Usage:
    ./raft [options] [peers ...]

Start a Raft server. The server will treat each operand in *peers* as a "peer"
Raft server part of the same cluster. These arguments should be in the format
"hostname:listen_port:remote_port", e.g. "12.34.56.67:4000:4001".

Example:
    Start a three server Raft cluster.

        ./raft --id alice 127.0.0.1:4000:4010 127.0.0.1:4001:4020
        ./raft --id bob 127.0.0.1:4010:4000 127.0.0.1:4011:4021
        ./raft --id carol 127.0.0.1:4020:4001 127.0.0.1:4021:4011
)";
