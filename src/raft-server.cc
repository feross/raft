#include "raft-server.h"

static const string& getServerStateString(ServerState server_state) {
    return ServerStateStrings[server_state];
}

RaftServer::RaftServer(const string& server_id, Storage storage, unsigned short port,
        unsigned short connect_port) : server_id(server_id),
        storage(storage) {

    Peer *peer = new Peer(port, "127.0.0.1", connect_port,
        [this](Peer* peer, char* raw_message, int raw_message_len) {
            HandlePeerMessage(peer, raw_message, raw_message_len);
        });
    peers.push_back(peer);

    electionTimer = new Timer(ELECTION_MIN_TIMEOUT, ELECTION_MAX_TIMEOUT, [this]() {
        HandleElectionTimer();
    });

    leaderTimer = new Timer(LEADER_HEARTBEAT_INTERVAL, [this]() {
        HandleLeaderTimer();
    });
}

void RaftServer::HandleElectionTimer() {
    lock_guard<mutex> lock(stateMutex);
    if (server_state == Leader) {
        return;
    }
    TransitionServerState(Candidate);
}

void RaftServer::HandleLeaderTimer() {
    lock_guard<mutex> lock(stateMutex);
    leaderTimer->Reset();
    if (server_state != Leader) {
        return;
    }
    for (Peer* peer: peers) {
        SendAppendentriesRequest(peer);
    }
}

void RaftServer::HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len) {
    lock_guard<mutex> lock(stateMutex);
    PeerMessage message;
    message.ParseFromString(string(raw_message, raw_message_len));
    cout << "RECEIVE: " << Util::ProtoDebugString(message) << endl;

    if (message.term() > storage.current_term()) {
        TransitionCurrentTerm(message.term());
        TransitionServerState(Follower);
    }

    switch (message.type()) {
        case PeerMessage::APPENDENTRIES_REQUEST:
            if (message.term() < storage.current_term()) {
                SendAppendentriesResponse(peer, false);
                return;
            }
            SendAppendentriesResponse(peer, true);
            electionTimer->Reset();
            return;

        case PeerMessage::APPENDENTRIES_RESPONSE:
            if (message.term() < storage.current_term()) {
                // Drop responses with an outdated term; they indicate this
                // response is for a request from a previous term.
                return;
            }
            return;

        case PeerMessage::REQUESTVOTE_REQUEST:
            if (message.term() < storage.current_term()) {
                SendRequestvoteResponse(peer, false);
                return;
            }
            if (storage.voted_for() != "" &&
                storage.voted_for() != message.server_id()) {
                SendRequestvoteResponse(peer, false);
                return;
            }
            storage.set_voted_for(message.server_id());
            SendRequestvoteResponse(peer, true);
            electionTimer->Reset();
            return;

        case PeerMessage::REQUESTVOTE_RESPONSE:
            if (message.term() < storage.current_term()) {
                // Drop responses with an outdated term; they indicate this
                // response is for a request from a previous term.
                return;
            }
            if (message.vote_granted()) {
                ReceiveVote(message.server_id());
            }
            return;

        default:
            cerr << "Unexpected message type: " <<
                Util::ProtoDebugString(message) << endl;
            throw RaftServerException();
    }
}

PeerMessage RaftServer::CreateMessage() {
    PeerMessage message;
    message.set_term(storage.current_term());
    message.set_server_id(server_id);
    return message;
}

void RaftServer::SendMessage(Peer *peer, PeerMessage &message) {
    string message_string;
    message.SerializeToString(&message_string);
    cout << "SEND: " << Util::ProtoDebugString(message) << endl;
    const char* message_cstr = message_string.c_str();
    peer->SendMessage(message_cstr, message_string.size());
}

void RaftServer::SendAppendentriesRequest(Peer *peer) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::APPENDENTRIES_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendAppendentriesResponse(Peer *peer, bool success) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::APPENDENTRIES_RESPONSE);
    message.set_success(success);
    SendMessage(peer, message);
}

void RaftServer::SendRequestvoteRequest(Peer *peer) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::REQUESTVOTE_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendRequestvoteResponse(Peer *peer, bool vote_granted) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::REQUESTVOTE_RESPONSE);
    message.set_vote_granted(vote_granted);
    SendMessage(peer, message);
}

void RaftServer::TransitionCurrentTerm(int term) {
    cout << "TERM: " << storage.current_term() << " -> " << term << endl;
    storage.set_current_term(term);
    // When updating the term, reset who we voted for
    storage.set_voted_for("");
}

void RaftServer::TransitionServerState(ServerState new_state) {
    cout << "STATE: " << getServerStateString(server_state) << " -> " <<
        getServerStateString(new_state) << endl;

    server_state = new_state;

    switch (new_state) {
        case Follower:
            return;

        case Candidate:
            TransitionCurrentTerm(storage.current_term() + 1);
            ReceiveVote(server_id);
            for (Peer* peer: peers) {
                SendRequestvoteRequest(peer);
            }
            electionTimer->Reset();
            return;

        case Leader:
            return;

        default:
            cerr << "Bad state transition to " << new_state << endl;
            throw RaftServerException();
    }
}

void RaftServer::ReceiveVote(string server_id) {
    votes[server_id] = true;

    int vote_count = 0;
    for(auto const& [_, vote_granted]: votes) {
        if (vote_granted) vote_count += 1;
    }

    int server_count = peers.size() + 1;
    int majority_threshold = (server_count / 2) + 1;
    if (vote_count >= majority_threshold) {
        TransitionServerState(Leader);
    }
}
