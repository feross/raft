#include "raft-server.h"

static const string& getServerStateString(ServerState server_state) {
    return ServerStateStrings[server_state];
}

RaftServer::RaftServer(const string& server_id, Storage storage, unsigned short port,
        unsigned short connect_port) : server_id(server_id),
        storage(storage) {

    Peer *peer = new Peer(port, "127.0.0.1", connect_port,
        [this](Peer* peer, char* raw_message, int raw_message_len) {
            HandleMessage(peer, raw_message, raw_message_len);
        });
    peers.push_back(peer);

    timer = new Timer(ELECTION_MIN_TIMEOUT, ELECTION_MAX_TIMEOUT, [this]() {
        TransitionServerState(Candidate);
    });
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
    const char* message_cstr = message_string.c_str();
    cout << "SEND: " << Util::ProtoDebugString(message) << endl;
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

void RaftServer::HandleMessage(Peer* peer, char* raw_message, int raw_message_len) {
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
            return;
        case PeerMessage::APPENDENTRIES_RESPONSE:
            if (message.term() < storage.current_term()) {
                return;
            }
            return;
        case PeerMessage::REQUESTVOTE_REQUEST:
            if (message.term() < storage.current_term()) {
                SendRequestvoteResponse(peer, false);
                return;
            }
            if (storage.voted_for() == "" ||
                storage.voted_for() == message.server_id()) {
                SendRequestvoteResponse(peer, true);
            } else {
                SendRequestvoteResponse(peer, false);
            }
            return;
        case PeerMessage::REQUESTVOTE_RESPONSE:
            if (message.term() < storage.current_term()) {
                return;
            }
            return;
        default:
            cerr << "Unexpected message: " << Util::ProtoDebugString(message) << endl;
            return;
    }
}

void RaftServer::TransitionCurrentTerm(int term) {
    storage.set_current_term(term);

    // When updating the term, reset who we voted for
    storage.set_voted_for("");
}

void RaftServer::TransitionServerState(ServerState new_state) {
    ServerState prev_state = server_state;
    server_state = new_state;
    cout << "STATE: " << getServerStateString(prev_state) << " -> " <<
        getServerStateString(new_state) << endl;

    switch (new_state) {
        case Follower:
            return;
        case Candidate:
            TransitionCurrentTerm(storage.current_term() + 1);
            ReceiveVote(server_id);
            for (Peer* peer: peers) {
                SendRequestvoteRequest(peer);
            }
            timer->Reset();
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

    if (vote_count >= (peers.size() / 2) + 1) {
        TransitionServerState(Leader);
    }
}
