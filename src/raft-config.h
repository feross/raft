/**
 * A class that parses a configuration file into a structured form that is
 * useful to servers and clients in a Raft cluster. The simplest possible config
 * file looks like this:
 *
 *     127.0.0.1 4000
 *
 * This defines a server that listens for client requests on `127.0.0.1` and
 * port 4000.
 *
 * Here is a sample config file for a three server cluster:
 *
 *     127.0.0.1 4000 4001 4002
 *     127.0.0.1 5000 5001 5002
 *     127.0.0.1 6000 6001 6002
 *
 * Each line represents a separate server. Since there are three servers in the
 * cluster, each server needs to open two additional ports to receive
 * connections from the other servers in the cluster.
 *
 * Lines which are prefixed with a hash character (#) are considered comments.
 *
 * See the Peer class documentation for more information about why one listening
 * port per peer is required.
 */

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
 * Server information used by clients. The ip address and listening port that
 * clients should connect to when sending commands. Other servers in the cluster
 * also use this information for redirecting clients to the current leader.
 */
struct ServerInfo {
    string ip_addr;
    unsigned short port;
};

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
        /**
         * Create a new config file parser.
         *
         * @param config_path Filesystem path where the config file is located
         */
        RaftConfig(string config_path);

        /**
         * Read, parse, and process the config file specified in the
         * constructor. The expected file format is described in the comment
         * at the top of this file.
         *
         * The given `my_server_id` parameter is used to determine which peer
         * connection information should be used to connect to the other peers
         * in the cluster, as this information is different for each peer
         * because of our unique design where each server creates a listening
         * port for each of the other peers in the cluster. See the Peer class
         * documentation for more information about this design.
         *
         * @throws RaftConfigException
         *
         * @param my_server_id
         */
        void parse(int my_server_id);

        /**
         * Return the cluster server information as a vector, one ServerInfo
         * struct entry per server in the cluster.
         *
         * @return vector of ServerInfo structs
         */
        vector<ServerInfo> get_server_infos();

        /**
         * Return the peer connection information as a vector, one PeerInfo
         * struct entry per peer in the cluster. Note: there will be one less
         * entry in this vector than there are servers in the cluster because
         * there is no connection information for a server to connect to itself.
         *
         * @return vector of PeerInfo structs
         */
        vector<PeerInfo> get_peer_infos();
    private:
        /**
         * Filesystem path where the config file is located
         */
        string config_path;

        vector<ServerInfo> server_infos;
        vector<PeerInfo> peer_infos;
};
