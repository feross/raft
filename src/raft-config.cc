#include "raft-config.h"

RaftConfig::RaftConfig(string config_path) : config_path(config_path) {}

void RaftConfig::parse(int my_server_id) {
    fstream input(config_path, ios::in);

    vector<vector<unsigned short>> servers_ports;

    string line;
    while (getline(input, line)) {
        Util::Trim(line);
        if (input.eof()) {
            // End-of-file received; quit client
            break;
        }
        if (line.empty()) {
            // Ignore empty lines
            continue;
        }
        if (line[0] == '#') {
            // Ignore lines that start with '#' (comments)
            continue;
        }
        vector<string> line_tokens = Util::StringSplit(line, " ");

        if (line_tokens.size() < 2) {
            throw RaftConfigException("Invalid config line: " + line);
        }

        ServerInfo server_info = {
            line_tokens[0],
            (unsigned short) stoi(line_tokens[1], NULL)
        };
        server_infos.push_back(server_info);

        vector<unsigned short> server_ports;
        server_ports.resize(line_tokens.size() - 2);

        transform(line_tokens.begin() + 2, line_tokens.end(),
            server_ports.begin(),
            [](string& str) -> unsigned short {
                return (unsigned short) stoi(str, NULL);
            });

        servers_ports.push_back(server_ports);
    }

    int num_servers = servers_ports.size();
    for (int server_id = 0; server_id < num_servers; server_id++) {
        if (server_id == my_server_id) {
            continue;
        }

        PeerInfo peer_info;
        peer_info.my_listen_port = servers_ports[my_server_id][peer_infos.size()];
        peer_info.destination_ip_addr = server_infos[server_id].ip_addr;

        vector<unsigned short> dest_server_ports = servers_ports[server_id];

        int port_index = 0;
        for (int i = 0; i < num_servers; i++) {
            if (i == server_id) {
                continue;
            }
            if (i == my_server_id) {
                peer_info.destination_port = dest_server_ports[port_index];
                break;
            }
            port_index += 1;
        }

        peer_infos.push_back(peer_info);
    }
}

vector<ServerInfo> RaftConfig::get_server_infos() {
    for (ServerInfo server_info: server_infos) {
        debug("server: %s:%d", server_info.ip_addr.c_str(), server_info.port);
    }
    return server_infos;
}

vector<PeerInfo> RaftConfig::get_peer_infos() {
    for (PeerInfo peer_info: peer_infos) {
        debug("peer info: %s:%d:%d", peer_info.destination_ip_addr.c_str(),
            peer_info.my_listen_port, peer_info.destination_port);
    }
    return peer_infos;
}
