#include "raft-server.h"

static const string& getServerStateString(ServerState server_state) {
    return ServerStateStrings[server_state];
}

RaftServer::RaftServer(const string& server_name, Storage storage, unsigned short port,
        unsigned short connect_port) : server_name(server_name),
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

proto::PeerMessage RaftServer::CreateMessage() {
    proto::PeerMessage message;
    message.set_term(storage.current_term());
    message.set_sender_id(server_name);
    return message;
}

void RaftServer::SendMessage(Peer *peer, proto::PeerMessage &message) {
    string message_string;
    message.SerializeToString(&message_string);
    const char* message_cstr = message_string.c_str();
    cout << "SEND: " << Util::ProtoDebugString(message) << endl;
    peer->SendMessage(message_cstr, message_string.size());
}

void RaftServer::SendAppendentriesRequest(Peer *peer) {
    proto::PeerMessage message = CreateMessage();
    message.set_type(proto::PeerMessage::APPENDENTRIES_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendAppendentriesResponse(Peer *peer, bool success) {
    proto::PeerMessage message = CreateMessage();
    message.set_type(proto::PeerMessage::APPENDENTRIES_RESPONSE);
    message.set_success(success);
    SendMessage(peer, message);
}

void RaftServer::SendRequestvoteRequest(Peer *peer) {
    proto::PeerMessage message = CreateMessage();
    message.set_type(proto::PeerMessage::REQUESTVOTE_REQUEST);
    SendMessage(peer, message);
}

void RaftServer::SendRequestvoteResponse(Peer *peer, bool vote_granted) {
    proto::PeerMessage message = CreateMessage();
    message.set_type(proto::PeerMessage::REQUESTVOTE_RESPONSE);
    message.set_vote_granted(vote_granted);
    SendMessage(peer, message);
}

void RaftServer::HandleMessage(Peer* peer, char* raw_message, int raw_message_len) {
    proto::PeerMessage message;
    message.ParseFromString(string(raw_message, raw_message_len));
    cout << "RECEIVE: " << Util::ProtoDebugString(message) << endl;

    switch (message.type()) {
        case proto::PeerMessage::APPENDENTRIES_REQUEST:
            if (message.term() < storage.current_term()) {
                SendAppendentriesResponse(peer, false);
            } else {
                SendAppendentriesResponse(peer, true);
            }
            return;
        case proto::PeerMessage::APPENDENTRIES_RESPONSE:
            return;
        case proto::PeerMessage::REQUESTVOTE_REQUEST:
            if (message.term() < storage.current_term()) {
                SendRequestvoteResponse(peer, false);
                return;
            }
            if (storage.voted_for() == "" ||
                storage.voted_for() == message.sender_id()) {
                SendRequestvoteResponse(peer, true);
            } else {
                SendRequestvoteResponse(peer, false);
            }
            return;
        case proto::PeerMessage::REQUESTVOTE_RESPONSE:
            return;
        default:
            cerr << "Unexpected message: " << Util::ProtoDebugString(message) << endl;
            return;
    }
}

void RaftServer::TransitionCurrentTerm(int term) {
    storage.set_current_term(term);
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
            // TODO: Vote for self
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
