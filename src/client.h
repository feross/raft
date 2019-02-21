#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "arguments.h"
#include "log.h"
#include "raft-config.h"

static const string INTRO_TEXT =
R"(Raft Client

Usage:
    ./client [options] [peers ...]
)";
