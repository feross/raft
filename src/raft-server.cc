#include "raft-server.h"

RaftServer::RaftServer(int server_id, vector<ServerInfo> server_infos,
    vector<PeerInfo> peer_infos) : server_id(server_id),
    server_infos(server_infos), peer_infos(peer_infos),
    storage(to_string(server_id) + STORAGE_NAME_SUFFIX),
    persistent_log((to_string(server_id) + STORAGE_NAME_SUFFIX).c_str()),
    committed_index(0) {}

void RaftServer::Run() {
    storage.Load();

    info("TERM: %d", storage.current_term());
    info("STATE: %s", ServerStateStrings[Follower].c_str());

    for (int i = 0; i < peer_infos.size(); i++) {
        PeerInfo peer_info = peer_infos[i];
        Peer *peer = new Peer(peer_info.my_listen_port,
            peer_info.destination_ip_addr, peer_info.destination_port,
            [this](Peer* peer, char* raw_message, int raw_message_len) {
                HandlePeerMessage(peer, raw_message, raw_message_len);
            });
        peer->id = i;
        peers.push_back(peer);
    }

    election_timer = new Timer(ELECTION_MIN_TIMEOUT, ELECTION_MAX_TIMEOUT, [this]() {
        HandleElectionTimer();
    });

    leader_timer = new Timer(LEADER_HEARTBEAT_INTERVAL, [this]() {
        HandleLeaderTimer();
    });

    unsigned short listen_port = server_infos[server_id].port;
    client_server = new ClientServer([this](char * command) -> int {
        return HandleClientCommand(command);
    });
    client_server->Listen(listen_port);
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
    CheckForCommittedEntries();
}

int RaftServer::HandleClientCommand(char * command) {
    lock_guard<mutex> lock(server_mutex);
    assert(server_state == Leader);

    info("Client command: %s", command);

    int prev_last_log_index = persistent_log.LastLogIndex();

    // Append log entry
    int log_entry_len = strlen(command) + 1;
    char log_entry_buffer[log_entry_len + sizeof(int)];
    memcpy(log_entry_buffer, &log_entry_len, sizeof(int));
    memcpy(log_entry_buffer + sizeof(int), command, log_entry_len);
    persistent_log.AddLogEntry(log_entry_buffer, log_entry_len + sizeof(int));

    int last_log_index = persistent_log.LastLogIndex();
    info("Added to log (prev index %d, current index %d)", prev_last_log_index, last_log_index);
    return last_log_index;
}

void RaftServer::HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len) {
    lock_guard<mutex> lock(server_mutex);
    PeerMessage message;
    message.ParseFromString(string(raw_message, raw_message_len));

    debug("RECEIVE: %s", Util::ProtoDebugString(message).c_str());

    if (message.term() > storage.current_term()) {
        TransitionCurrentTerm(message.term());
        TransitionServerState(Follower);
        client_server->StartRedirecting(&server_infos[message.server_id()]);
    }

    switch (message.type()) {
        case PeerMessage::APPENDENTRIES_REQUEST: {
            if (message.term() < storage.current_term()) {
                SendAppendEntriesResponse(peer, false, message.prev_log_index() + 1);
                return;
            }
            if (server_state == Candidate && message.term() == storage.current_term()) {
                // Candidate recognizes another candidate has won election
                TransitionServerState(Follower);
                client_server->StartRedirecting(&server_infos[message.server_id()]);
            }


            int largest_log_index = persistent_log.LastLogIndex();
            if (largest_log_index < message.prev_log_index()) {
                SendAppendEntriesResponse(peer, false, message.prev_log_index() + 1);
                return;
            }
            info("%s", "NUMBER 1");

            struct LogEntry compare_entry =
                persistent_log.GetLogEntryByIndex(message.prev_log_index());
            int compare_entry_term = *(int *)compare_entry.data;
            if (compare_entry_term != message.prev_log_term()) {
                SendAppendEntriesResponse(peer, false, message.prev_log_index() + 1);
                return;
            }

            while (largest_log_index > message.prev_log_index()) {
                if (persistent_log.RemoveLogEntry() != true) {
                    warn("%s", "failed to remove an entry from log");
                }
                largest_log_index -= 1;
                // should == largest_log_index = persistent_log.LastLogIndex();
            }
            if (message.entries_size() == 0) {
                SendAppendEntriesResponse(peer, true, message.prev_log_index() + 1);
                election_timer->Reset();
                return;
            } else {
                string log_entry = message.entries(0);
                int log_entry_len = log_entry.length();
                char log_entry_buffer[log_entry_len + sizeof(int)];

                info("%s", "NUMBER 2");
                memcpy(log_entry_buffer, &log_entry_len, sizeof(int));
                memcpy(log_entry_buffer + sizeof(int), log_entry.c_str(), log_entry_len);
                persistent_log.AddLogEntry(log_entry_buffer,
                    log_entry_len + sizeof(int));
                info("%s", "NUMBER 3");

                SendAppendEntriesResponse(peer, true, message.prev_log_index() + 1);
                election_timer->Reset();
                return;
            }
        }

        case PeerMessage::APPENDENTRIES_RESPONSE: {
            if (message.term() < storage.current_term()) {
                // Drop responses with an outdated term; they indicate this
                // response is for a request from a previous term.
                return;
            }
            debug("append response: success %d, %d", message.success(), message.appended_log_index());

            if (message.success()) {
                if (message.appended_log_index() >= peer_next_indexes[peer->id]) {
                    peer_next_indexes[peer->id] = message.appended_log_index() + 1;
                    peer_match_indexes[peer->id] = message.appended_log_index();

            warn("%s", "NUMBER 5");

                    //might now have new committed entry
                    CheckForCommittedEntries();
                }
            } else {
                peer_next_indexes[peer->id] = message.appended_log_index() - 1;
            }
            if (peer_next_indexes[peer->id] <= persistent_log.LastLogIndex()){
                SendAppendEntriesRequest(peer); //still need to catch up
            }
            return;
        }

        case PeerMessage::REQUESTVOTE_REQUEST: {
            if (message.term() < storage.current_term()) {
                info("%s", "vote rejected: term too old");
                SendRequestVoteResponse(peer, false);
                return;
            }
            if (storage.voted_for() != -1 &&
                storage.voted_for() != message.server_id()) {
                // Voted for another server already
                info("%s", "vote rejected: already voted for someone else");
                SendRequestVoteResponse(peer, false);
                return;
            }

            struct LogEntry prev_entry =
                persistent_log.GetLogEntryByIndex(message.prev_log_index());
            int prev_entry_term = *(int *)prev_entry.data;

            if (message.last_log_term() > prev_entry_term) {
                storage.set_term_and_voted(storage.current_term(), message.server_id());
                info("%s", "vote accepted: term was higher");
                SendRequestVoteResponse(peer, true);
                election_timer->Reset();
                return;
            }
            if (message.last_log_term() == prev_entry_term &&
                message.last_log_index() >= persistent_log.LastLogIndex()) {
                storage.set_term_and_voted(storage.current_term(), message.server_id());
                info("%s", "vote accepted: term was equal & index >=");
                SendRequestVoteResponse(peer, true);
                election_timer->Reset();
                return;
            }
            info("vote rejected for misc reason their term: %d my term: %d", message.term(), storage.current_term());
            info("their message term: %d ", message.last_log_term());
            info("their index %d my index: %d", message.last_log_index(), persistent_log.LastLogIndex());
            SendRequestVoteResponse(peer, false);
            return;
        }

        case PeerMessage::REQUESTVOTE_RESPONSE: {
            if (message.term() < storage.current_term()) {
                // Drop responses with an outdated term; they indicate this
                // response is for a request from a previous term.
                return;
            }
            if (message.vote_granted()) {
                ReceiveVote(message.server_id());
            }
            return;
        }

        default:
            warn("Unexpected message type: %s", message.type());
    }
}

void RaftServer::CheckForCommittedEntries() {
    info("%s", "CheckForCommittedEntries");
    while (true) {
        int matches = 1; // Leader always has the latest log entry
        for (int i = 0; i < peer_match_indexes.size(); i++) {
            if (peer_match_indexes[i] > committed_index) {
                matches += 1;
            }
        }
        int majority_threshold = (server_infos.size() / 2) + 1;
        if (matches >= majority_threshold) {
            struct LogEntry ent = persistent_log.
                GetLogEntryByIndex(committed_index + 1);
            int term = *(int *)ent.data;
            info("CheckForCommittedEntries 1 (term: %d) (command: %s)", term, ent.data + 4);
            if (term == storage.current_term()) {
                info("%s", "CheckForCommittedEntries 2");
                char * data = ent.data + sizeof(int);
                string response = state_machine.Apply(string(data));
                client_server->RespondToClient(committed_index + 1, response);
                storage.set_last_applied(committed_index + 1);
                committed_index += 1;
                continue;
            }
        }
        warn("%s", "NUMBER 6");

        break;
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
    int next_index = peer_next_indexes[peer->id];
    bool empty_body = false;
    if (next_index > persistent_log.LastLogIndex()) {
        debug("%s", "empty body of append entries");
        next_index = persistent_log.LastLogIndex() + 1;
        empty_body = true;
    }

    struct LogEntry prev_entry = persistent_log.GetLogEntryByIndex(next_index-1);
    int prev_entry_term = *(int *)prev_entry.data;

    message.set_prev_log_term(prev_entry_term);
    message.set_prev_log_index(next_index - 1);
    message.set_leader_commit(committed_index);
    if (!empty_body) {
        debug("%s", "Append Entry is non-empty");
        struct LogEntry cur_entry = persistent_log.GetLogEntryByIndex(next_index);
        message.add_entries(cur_entry.data + sizeof(int),
            cur_entry.len - sizeof(int));
    }
    SendMessage(peer, message);
}

void RaftServer::SendAppendEntriesResponse(Peer *peer, bool success,
        int appended_log_index) {
    PeerMessage message = CreateMessage(PeerMessage::APPENDENTRIES_RESPONSE);
    message.set_appended_log_index(appended_log_index);
    message.set_success(success);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteRequest(Peer *peer) {
    PeerMessage message = CreateMessage(PeerMessage::REQUESTVOTE_REQUEST);
    int last_log_entry_index = persistent_log.LastLogIndex();
    struct LogEntry prev_entry = persistent_log.GetLogEntryByIndex(last_log_entry_index);
    int last_log_entry_term = *(int *)prev_entry.data;
    message.set_last_log_index(last_log_entry_index);
    message.set_last_log_term(last_log_entry_term);
    SendMessage(peer, message);
}

void RaftServer::SendRequestVoteResponse(Peer *peer, bool vote_granted) {
    PeerMessage message = CreateMessage(PeerMessage::REQUESTVOTE_RESPONSE);
    message.set_vote_granted(vote_granted);
    SendMessage(peer, message);
}

void RaftServer::TransitionCurrentTerm(int term) {
    info("TERM: %d -> %d", storage.current_term(), term);
    // When updating the term, reset who we voted for
    storage.set_term_and_voted(term, -1);
    votes.clear();
    info("%s","Done transitioning");
}

void RaftServer::TransitionServerState(ServerState new_state) {
    const char* old_state_str = ServerStateStrings[server_state].c_str();
    const char* new_state_str = ServerStateStrings[new_state].c_str();
    info("STATE: %s -> %s", old_state_str, new_state_str);

    server_state = new_state;

    switch (new_state) {
        case Follower: {
            return;
        }
        case Candidate: {
            TransitionCurrentTerm(storage.current_term() + 1);

            // Candidate server votes for itself
            storage.set_term_and_voted(storage.current_term(), server_id);
            ReceiveVote(server_id);

            for (Peer* peer: peers) {
                SendRequestVoteRequest(peer);
            }

            election_timer->Reset();
            return;
        }
        case Leader: {
            int next_log_index = persistent_log.LastLogIndex() + 1;
            int num_servers = server_infos.size();
            peer_next_indexes.clear();
            peer_match_indexes.clear();
            for (int i = 0; i < num_servers; i++) {
                peer_next_indexes.push_back(next_log_index);
                peer_match_indexes.push_back(0);
            }

            client_server->StartServing();
            return;
        }
    }
}

void RaftServer::ReceiveVote(int server_id) {
    if (server_state == Leader) {
        return;
    }

    votes.insert(server_id);

    int majority_threshold = (server_infos.size() / 2) + 1;
    if (votes.size() >= majority_threshold) {
        // This server won the election
        TransitionServerState(Leader);
    }
            info("%s", "Now what");
}
