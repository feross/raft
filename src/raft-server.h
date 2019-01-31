#ifndef _RAFT_SERVER_H_
#define _RAFT_SERVER_H_

#include <vector>

#include "peer.h"
#include "peermessage.pb.h"
#include "storage.h"
#include "timer.h"
#include "util.h"

enum ServerState { Follower, Candidate, Leader };

static const int ELECTION_MIN_TIMEOUT = 5'000;
static const int ELECTION_MAX_TIMEOUT = 10'000;

class RaftServer {
    public:
        RaftServer(const string& server_name, Storage storage, unsigned short port,
                unsigned short connect_port);
    private:
        proto::PeerMessage CreateMessage();
        void SendMessage(Peer *peer, proto::PeerMessage &message);
        void SendAppendentriesRequest(Peer *peer);
        void SendAppendentriesResponse(Peer *peer, bool success);
        void SendRequestvoteRequest(Peer *peer);
        void SendRequestvoteResponse(Peer *peer, bool vote_granted);
        void HandleMessage(Peer* peer, char* raw_message, int raw_message_len);
        void TransitionCurrentTerm(int term);
        void TransitionServerState(ServerState new_state);

        const string& server_name;
        Storage storage;

        ServerState server_state = Follower;
        vector<Peer*> peers;
        Timer *timer;
};

#endif
