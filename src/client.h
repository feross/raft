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

const static int MAX_CLIENT_RETRIES = 30;
const static duration CLIENT_RETRY_DELAY = seconds(3); // seconds

static vector<ServerInfo> server_infos;
static ServerInfo active_server_info;

static const string INTRO_TEXT =
R"(Raft Client

Usage:
    ./client [options] [peers ...]
)";
