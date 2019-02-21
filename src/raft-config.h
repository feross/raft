#pragma once

#include <fstream>
#include <cstdio>
#include <string>

#include "log.h"
#include "util.h"

using namespace std;

const string DEFAULT_CONFIG_PATH = "./config";
const int MAX_CONFIG_FILE_SIZE = 1'000'000; // bytes

/**
 * Peer connection information. Describes a peer that this server should connect
 * to. Peers are specified via destination_ip_addr and destination_port. The
 * port on which this server will listen for a incoming connection from this
 * peer is specified as my_listen_port.
 */
struct PeerInfo {
    string destination_ip_addr;
    unsigned short destination_port;
    unsigned short my_listen_port;
};

struct ServerInfo {
    string ip_addr;
    unsigned short port;
};

class RaftConfigException : public exception {
    public:
        RaftConfigException(const string& message): message(message) {}
        RaftConfigException(const char* message): message(message) {}
        const char* what() const noexcept { return message.c_str(); }
    private:
        string message;
};

class RaftConfig {
    public:
        RaftConfig(string config_path);
        void parse(int my_server_id);
        vector<ServerInfo> get_server_infos();
        vector<PeerInfo> get_peer_infos();
    private:
        string config_path;
        vector<ServerInfo> server_infos;
        vector<PeerInfo> peer_infos;
};
