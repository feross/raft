#include "raft-server.h"

RaftServer::RaftServer(const string& server_id,
    vector<struct PeerInfo> peer_info_vector, int client_listen_port) :
    server_id(server_id), storage(server_id + STORAGE_NAME_SUFFIX) {

    info("TERM: %d", storage.current_term());

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

    client_server = new ClientServer([this](char * command) {
        HandleClientCommand(command);
    });
    client_server->Listen(client_listen_port);
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

void RaftServer::HandleClientCommand(char * command) {
    lock_guard<mutex> lock(server_mutex);
    info("%s", "HandleClientCommand");
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
            if (message.term() == storage.current_term()) {
                // Candidate recognizes another candidate has won election
                TransitionServerState(Follower);
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
                // Voted for another server already
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


void RaftServer::SendMessage(Peer *peer, PeerMessage &message) {
    debug("SEND: %s", Util::ProtoDebugString(message).c_str());
    string message_string;
    message.SerializeToString(&message_string);
    peer->SendMessage(message_string.c_str(), message_string.size());
}

PeerMessage RaftServer::CreateMessage(PeerMessage_Type message_type) {
    PeerMessage message;
    message.set_type(message_type);
    message.set_term(storage.current_term());
    message.set_server_id(server_id);
    return message;
}

void RaftServer::SendAppendEntriesRequest(Peer *peer) {
    PeerMessage message = CreateMessage(PeerMessage::APPENDENTRIES_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendAppendEntriesResponse(Peer *peer, bool success) {
    PeerMessage message = CreateMessage(PeerMessage::APPENDENTRIES_RESPONSE);
    message.set_success(success);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteRequest(Peer *peer) {
    PeerMessage message = CreateMessage(PeerMessage::REQUESTVOTE_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteResponse(Peer *peer, bool vote_granted) {
    PeerMessage message = CreateMessage(PeerMessage::REQUESTVOTE_RESPONSE);
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
    if (server_state == new_state) {
        // Do not transition if already in same state
        return;
    }

    const char* old_state_str = ServerStateStrings[server_state].c_str();
    const char* new_state_str = ServerStateStrings[new_state].c_str();
    info("STATE: %s -> %s", old_state_str, new_state_str);

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
    if (server_state == Leader) {
        return;
    }

    votes.insert(server_id);

    int server_count = peers.size() + 1;
    int majority_threshold = (server_count / 2) + 1;

    if (votes.size() >= majority_threshold) {
        // This server won the election
        TransitionServerState(Leader);
    }
}
