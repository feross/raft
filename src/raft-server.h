#ifndef _RAFT_SERVER_H_
#define _RAFT_SERVER_H_

#include <map>
#include <vector>

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
        RaftServer(const string& server_id, Storage storage, unsigned short port,
                unsigned short connect_port);
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

#endif
