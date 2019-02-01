#pragma once

#include <map>
#include <vector>

#include "log.h"
#include "peer.h"
#include "peer.pb.h"
#include "storage.h"
#include "timer.h"
#include "util.h"

using namespace proto;

enum ServerState { Follower, Candidate, Leader };
static const string ServerStateStrings[] = { "Follower", "Candidate", "Leader" };

static const int ELECTION_MIN_TIMEOUT = 5'000;
static const int ELECTION_MAX_TIMEOUT = 10'000;
static const int LEADER_HEARTBEAT_INTERVAL = 2'000;

class RaftServerException : public exception {
    const char* what() const noexcept {
        return "Unexpected Raft server error";
    }
};

class RaftServer {
    public:
        /**
         * Start a server that implements Raft, an understandable consensus
         * protocol.
         *
         * The server expects to be given a server id, which is a friendly name
         * that identifies the server to other servers in the cluster, and a
         * vector of peer connection information which is used to connect to
         * other servers in the cluster and start corresponding listening
         * servers.
         *
         * For more information about the operation of Raft, see:
         * http://web.stanford.edu/~ouster/cgi-bin/papers/raft-extended.pdf
         *
         * @param server_id Friendly name to identify the server
         * @param peer_infos Vector of connection information for peer servers
         */
        RaftServer(const string& server_id, vector<struct PeerInfo> peer_infos);
    private:
        void HandleElectionTimer();
        void HandleLeaderTimer();
        void HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len);
        PeerMessage CreateMessage();
        void SendMessage(Peer *peer, proto::PeerMessage &message);
        void SendAppendentriesRequest(Peer *peer);
        void SendAppendentriesResponse(Peer *peer, bool success);
        void SendRequestvoteRequest(Peer *peer);
        void SendRequestvoteResponse(Peer *peer, bool vote_granted);
        void TransitionCurrentTerm(int term);
        void TransitionServerState(ServerState new_state);
        void ReceiveVote(string server_id);

        const string& server_id;
        Storage storage;

        ServerState server_state = Follower;
        vector<Peer*> peers;

        Timer *electionTimer;
        Timer *leaderTimer;

        map<string, bool> votes;
        mutex stateMutex;
};

struct PeerInfo {
    unsigned short my_listen_port;
    unsigned short destination_port;
    string destination_ip_addr;
};
