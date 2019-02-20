/**
 * A server that implements Raft, an understandable consensus protocol.
 *
 * For more information about the operation of Raft, see:
 * http://web.stanford.edu/~ouster/cgi-bin/papers/raft-extended.pdf
 */

#pragma once

#include <map>
#include <vector>

#include "log.h"
#include "peer.h"
#include "peer-message.pb.h"
#include "raft-storage.h"
#include "timer.h"
#include "util.h"

using namespace proto;

enum ServerState { Follower, Candidate, Leader };
static const string ServerStateStrings[] = { "Follower", "Candidate", "Leader" };

static const int ELECTION_MIN_TIMEOUT = 5'000; // milliseconds
static const int ELECTION_MAX_TIMEOUT = 10'000; // milliseconds
static const int LEADER_HEARTBEAT_INTERVAL = 2'000; // milliseconds

/**
 * Peer connection information. Describes a peer that this server should connect
 * to. Peers are specified via destination_ip_addr and destination_port. The
 * port on which this server will listen for a incoming connection from this
 * peer is specified as my_listen_port.
 */
struct PeerInfo {
    string destination_ip_addr;
    unsigned short destination_port;
    unsigned short my_listen_port;
};

class RaftServer {
    public:
        /**
         * Construct a Raft server.
         *
         * The server expects to be given a server id, which is a friendly name
         * that identifies the server to other servers in the cluster, and a
         * vector of peer connection information which is used to connect to
         * other servers in the cluster and start listening servers to receive
         * connections in return from the remote servers.
         *
         * @param server_id Friendly name to identify the server
         * @param peer_infos Vector of connection information for peer servers
         */
        RaftServer(const string& server_id, vector<struct PeerInfo> peer_infos);

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
        void SendAppendEntriesResponse(Peer *peer, bool success);

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
        void ReceiveVote(string server_id);

        /**
         * server_mutex prevents multiple handler functions from modifying the
         * server state at the same time. Specfically, we have timer threads and
         * peer threads which may call callback functions in the RaftServer
         * class and these should not ever run concurrently. All instance
         * methods that start with "Handle" should acquire this mutex for the
         * duration of their execution.
         */
        mutex server_mutex;

        /**
         * Friendly name that the server uses to identify itself to other
         * servers in the cluster, as well as to name thee persistent storage
         * file.
         */
        const string& server_id;

        ServerState server_state = Follower;
        RaftStorage storage;

        Timer *election_timer;
        Timer *leader_timer;

        vector<Peer*> peers;

        /**
         * Vote record to track which servers have voted for this server in the
         * current election. Maps server id strings to boolean values which
         * indicate an affirmative or negative election vote.
         */
        set<string> votes;
};
