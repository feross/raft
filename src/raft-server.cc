#include "raft-server.h"

RaftServer::RaftServer(const string& server_id,
    vector<struct PeerInfo> peer_info_vector) : server_id(server_id),
    storage(server_id + STORAGE_NAME_SUFFIX) {

    for (struct PeerInfo peer_info: peer_info_vector) {
        Peer *peer = new Peer(peer_info.my_listen_port,
            peer_info.destination_ip_addr, peer_info.destination_port,
            [this](Peer* peer, char* raw_message, int raw_message_len) {
                HandlePeerMessage(peer, raw_message, raw_message_len);
            });
        peers.push_back(peer);
    }

    election_timer = new Timer(ELECTION_MIN_TIMEOUT, ELECTION_MAX_TIMEOUT, [this]() {
        HandleElectionTimer();
    });

    leader_timer = new Timer(LEADER_HEARTBEAT_INTERVAL, [this]() {
        HandleLeaderTimer();
    });

    info("STARTING TERM: %d", storage.current_term());
}

void RaftServer::HandleElectionTimer() {
    lock_guard<mutex> lock(server_mutex);
    if (server_state == Leader) {
        return;
    }
    TransitionServerState(Candidate);
}

void RaftServer::HandleLeaderTimer() {
    lock_guard<mutex> lock(server_mutex);
    leader_timer->Reset();
    if (server_state != Leader) {
        return;
    }
    for (Peer* peer: peers) {
        SendAppendEntriesRequest(peer);
    }
}

void RaftServer::HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len) {
    lock_guard<mutex> lock(server_mutex);
    PeerMessage message;
    message.ParseFromString(string(raw_message, raw_message_len));

    debug("RECEIVE: %s", Util::ProtoDebugString(message).c_str());

    if (message.term() > storage.current_term()) {
        TransitionCurrentTerm(message.term());
        TransitionServerState(Follower);
    }

    switch (message.type()) {
        case PeerMessage::APPENDENTRIES_REQUEST:
            if (message.term() < storage.current_term()) {
                SendAppendEntriesResponse(peer, false);
                return;
            }
            SendAppendEntriesResponse(peer, true);
            election_timer->Reset();
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
                SendRequestVoteResponse(peer, false);
                return;
            }
            if (storage.voted_for() != "" &&
                storage.voted_for() != message.server_id()) {
                SendRequestVoteResponse(peer, false);
                return;
            }
            storage.set_voted_for(message.server_id());
            SendRequestVoteResponse(peer, true);
            election_timer->Reset();
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
            warn("Unexpected message type: %s", message.type());
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
    debug("SEND: %s", Util::ProtoDebugString(message).c_str());
    const char* message_cstr = message_string.c_str();
    peer->SendMessage(message_cstr, message_string.size());
}

void RaftServer::SendAppendEntriesRequest(Peer *peer) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::APPENDENTRIES_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendAppendEntriesResponse(Peer *peer, bool success) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::APPENDENTRIES_RESPONSE);
    message.set_success(success);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteRequest(Peer *peer) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::REQUESTVOTE_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteResponse(Peer *peer, bool vote_granted) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::REQUESTVOTE_RESPONSE);
    message.set_vote_granted(vote_granted);
    SendMessage(peer, message);
}


void RaftServer::TransitionCurrentTerm(int term) {
    info("TERM: %d -> %d", storage.current_term(), term);
    storage.set_current_term(term);
    // When updating the term, reset who we voted for
    storage.set_voted_for("");
    votes.clear();
}

void RaftServer::TransitionServerState(ServerState new_state) {
    info("STATE: %s -> %s", get_server_state_string(server_state),
        get_server_state_string(new_state));

    server_state = new_state;

    switch (new_state) {
        case Follower:
            return;

        case Candidate:
            TransitionCurrentTerm(storage.current_term() + 1);

            // Candidate server votes for itself
            storage.set_voted_for(server_id);
            ReceiveVote(server_id);

            for (Peer* peer: peers) {
                SendRequestVoteRequest(peer);
            }
            election_timer->Reset();
            return;

        case Leader:
            return;
    }
}

void RaftServer::ReceiveVote(string server_id) {
    votes[server_id] = true;
    if (server_state == Leader) return;

    int vote_count = 0;
    for (auto const& [_, vote_granted]: votes) {
        if (vote_granted) vote_count += 1;
    }

    int server_count = peers.size() + 1;
    int majority_threshold = (server_count / 2) + 1;
    if (vote_count >= majority_threshold) {
        TransitionServerState(Leader);
    }
}

static const char* get_server_state_string(ServerState server_state) {
    return ServerStateStrings[server_state].c_str();
}
