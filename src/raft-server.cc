#include "raft-server.h"

static const string& getServerStateString(ServerState server_state) {
    return ServerStateStrings[server_state];
}

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

    LOG(INFO) << "STARTING TERM: " << storage.current_term();
}

/**
 * Callback function invoked when we haven't received a valid message
 * within our timeout window.  Will cause election.
 */
void RaftServer::HandleElectionTimer() {
    lock_guard<mutex> lock(server_mutex);
    if (server_state == Leader) {
        return;
    }
    TransitionServerState(Candidate);
}

/**
 * Callback function invoked when we haven't send an AppendEntries heartbeat in
 * a while.  Will cause a heartbeat AppendEntries message if we are the leader.
 */
void RaftServer::HandleLeaderTimer() {
    lock_guard<mutex> lock(server_mutex);
    leader_timer->Reset();
    if (server_state != Leader) {
        return;
    }
    for (Peer* peer: peers) {
        SendAppendentriesRequest(peer);
    }
}

/**
 * Callback function used to process messages we have received from peers. Called
 * any time we receive a message from any peer, though shouldn't be happening
 * often except from leader (which should be processed in order anyway).
 *
 * @param peer - the peer connection from which we received the message
 * @param raw_message - pointer to message received from peer
 * @param raw_message_len - length of message received from peer
 */
void RaftServer::HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len) {
    lock_guard<mutex> lock(server_mutex);
    PeerMessage message;
    message.ParseFromString(string(raw_message, raw_message_len));

    LOG(DEBUG) << "RECEIVE: " << Util::ProtoDebugString(message);

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
            cerr << oslock << "Unexpected message type: " <<
                Util::ProtoDebugString(message) << endl << osunlock;
            throw RaftServerException();
    }
}

/**
 * Creates base message upon which all other message types are build.  Includes
 * the term & sender's id (useful for debugging even when not used directly)
 */
PeerMessage RaftServer::CreateMessage() {
    PeerMessage message;
    message.set_term(storage.current_term());
    message.set_server_id(server_id);
    return message;
}

/**
 * Sends a message formatted by protocol buffers to the specified peer.
 *
 * @param peer - the peer to send this message to
 * @param message - the message (any type) to send to this peer
 */
void RaftServer::SendMessage(Peer *peer, PeerMessage &message) {
    string message_string;
    message.SerializeToString(&message_string);
    LOG(DEBUG) << "SEND: " << Util::ProtoDebugString(message);
    const char* message_cstr = message_string.c_str();
    peer->SendMessage(message_cstr, message_string.size());
}

/**
 * Sends an AppendEntries request to the specified peer.  For now, the log
 * sent is empty & this is just used as a heartbeat.
 *
 * @param peer - the peer to send this AppendEntries request to
 */
void RaftServer::SendAppendentriesRequest(Peer *peer) {
    PeerMessage message = CreateMessage();
    message.set_type(PeerMessage::APPENDENTRIES_REQUEST);
    SendMessage(peer, message);
}

/**
 * Responds to an AppendEntries request.
 * For now, nearly always true unless old term.
 *
 * @param peer - the peer which we should send this response to
 * @param success - whether we accepted the AppendEntries request
 */
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

/**
 * Transition current term to a new term & sets voted_for to be empty,
 * and resets the votes that we have received
 */
void RaftServer::TransitionCurrentTerm(int term) {
    LOG(INFO) << "TERM: " << storage.current_term() << " -> " << term;
    storage.set_current_term(term);
    // When updating the term, reset who we voted for
    storage.set_voted_for("");
    votes.clear();
}

/**
 *
 */
void RaftServer::TransitionServerState(ServerState new_state) {
    LOG(INFO) << "STATE: " << getServerStateString(server_state) <<
        " -> " << getServerStateString(new_state);

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
                SendRequestvoteRequest(peer);
            }
            election_timer->Reset();
            return;

        case Leader:
            return;

        default:
            cerr << oslock << "Bad state transition to " << new_state << endl <<
                osunlock;
            throw RaftServerException();
    }
}

/**
 *
 */
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
