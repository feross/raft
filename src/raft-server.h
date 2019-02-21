/**
 * A server that implements Raft, an understandable consensus protocol.
 *
 * For more information about the operation of Raft, see:
 * http://web.stanford.edu/~ouster/cgi-bin/papers/raft-extended.pdf
 */

#pragma once

#include <map>
#include <vector>

#include "bash-state-machine.h"
#include "client-server.h"
#include "log.h"
#include "peer.h"
#include "peer-message.pb.h"
#include "raft-config.h"
#include "raft-storage.h"
#include "timer.h"
#include "util.h"
#include "persistent_log.h"

using namespace proto;

enum ServerState { Follower, Candidate, Leader };
static const string ServerStateStrings[] = { "Follower", "Candidate", "Leader" };

static const int ELECTION_MIN_TIMEOUT = 5'000; // milliseconds
static const int ELECTION_MAX_TIMEOUT = 10'000; // milliseconds
static const int LEADER_HEARTBEAT_INTERVAL = 2'000; // milliseconds

class RaftServer {
    public:
        /**
         * Construct a Raft server.
         *
         * The server expects to be given a server id, which is a friendly name
         * that identifies the server to other servers in the cluster, and a
         * vector of server information which defines which servers exist in the
         * cluster, and peer connection information which is used to establish
         * which incoming and outgoing ports will be used to communicate with
         * other servers in the cluster.
         *
         * @param server_id Friendly name to identify the server
         * @param server_infos Vector of server information
         * @param peer_infos Vector of connection information for peer servers
         */
        RaftServer(int server_id, vector<ServerInfo> server_infos,
            vector<PeerInfo> peer_infos);

        /**
         * Start running the server. Specifically, start the Raft protocol,
         * begin contacting peer servers, and accepting requests from clients.
         *
         * This method never returns.
         */
        void Run();

    private:
        /**
         * Callback function invoked when we haven't received a valid message
         * within our election timeout window.  Will cause election.
         */
        void HandleElectionTimer();

        /**
         * Callback function invoked at a period interval so that the server
         * can send an AppendEntries heartbeat, if it is the leader.
         */
        void HandleLeaderTimer();

        int HandleClientCommand(char * command);

        /**
         * Callback function used to process messages we receive from peers.
         * Called any time we receive a message from any peer, though shouldn't
         * happen in the normal case excpet when server is the leader.
         *
         * @param peer - the peer connection from which we received the message
         * @param raw_message - pointer to message received from peer
         * @param raw_message_len - length of message received from peer
         */
        void HandlePeerMessage(Peer* peer, char* raw_message, int raw_message_len);

        /**
         * Creates base message upon which all other message types are build.
         * Includes the current term and the sender id, which is the name of the
         * server (useful for debugging even when not used directly).
         *
         * The protocol buffer definition of the returned PeerMessage can be
         * found in peer.proto.
         *
         * @param message_type The raft message type
         * @return a PeerMessage protocol buffer
         */
        PeerMessage CreateMessage(PeerMessage_Type message_type);

        void CheckForCommittedEntries();

        void CommitEntries(int commit_index);

        /**
         * Sends a protocol buffer formatted message to the specified peer.
         *
         * @param peer - the peer to send this message to
         * @param message - the message (any type) to send to this peer
         */
        void SendMessage(Peer *peer, PeerMessage &message);

        /**
         * Sends an AppendEntries request to the specified peer. For now, the
         * log sent is empty and this is just used as a heartbeat.
         *
         * @param peer - the peer to send the AppendEntries request to
         */
        void SendAppendEntriesRequest(Peer *peer);

        /**
         * Responds to an AppendEntries request. For now, success is nearly
         * always true unless the sender has an old term.
         *
         * @param peer - the peer to send the AppendEntries response to
         * @param success - whether we accepted the AppendEntries request
         */
        void SendAppendEntriesResponse(Peer *peer, bool success,
            int appended_log_index);

        /**
         * Sends a RequestVote request to the specified peer.
         *
         * @param peer - the peer to send the RequestVote request to
         */
        void SendRequestVoteRequest(Peer *peer);

        /**
         * Send a RequestVoteResponse to the specified peer.

         * @param peer - the peer to send the RequestVoteResponse to
         * @param vote_granted whether we voted to elect the peer
         */
        void SendRequestVoteResponse(Peer *peer, bool vote_granted);

        /**
         * Transitions current term to a new term, sets voted_for to be empty,
         * and resets the votes that we have received in any past elections.
         * Assumes that the server mutex is held.
         */
        void TransitionCurrentTerm(int term);

        /**
         * Transitions the server to a new state (i.e. Follower, Candidate,
         * Leader). Assumes that the server mutex is held.
         *
         * @param new_state the new server state to enter
         */
        void TransitionServerState(ServerState new_state);

        /**
         * Record that a vote was received for us in the current election.
         * Assumes that the server mutex is held.
         *
         * @param server_id the server id of the server
         */
        void ReceiveVote(int server_id);

        /**
         * Friendly name that the server uses to identify itself to other
         * servers in the cluster, as well as to name thee persistent storage
         * file.
         */
        int server_id;

        vector<ServerInfo> server_infos;
        vector<PeerInfo> peer_infos;
        vector<Peer*> peers;

        ServerState server_state = Follower;
        RaftStorage storage;

        ClientServer *client_server;
        Timer *election_timer;
        Timer *leader_timer;

        /**
         * Log that is always in a consistent state, contains history of
         * appendentries requests
         */
        PersistentLog persistent_log;
        int committed_index;
        vector<int> peer_next_indexes;
        vector<int> peer_match_indexes;

        /**
         * Vote record to track which servers have voted for this server in the
         * current election. Maps server id strings to boolean values which
         * indicate an affirmative or negative election vote.
         */
        set<int> votes;

        /**
         * server_mutex prevents multiple handler functions from modifying the
         * server state at the same time. Specfically, we have timer threads and
         * peer threads which may call callback functions in the RaftServer
         * class and these should not ever run concurrently. All instance
         * methods that start with "Handle" should acquire this mutex for the
         * duration of their execution.
         */
        mutex server_mutex;

        BashStateMachine state_machine;
};
