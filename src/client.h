#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "arguments.h"
#include "log.h"

static const string INTRO_TEXT =
R"(Raft Client

Usage:
    ./client [options] [peers ...]
)";
