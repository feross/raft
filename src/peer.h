/**
 * Peer abstraction that enables sending/receiving messages to/from other
 * servers with a simple interface.
 */

#pragma once

#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

#include "log.h"

//TODO maybe template to allow to make specific if desired, but also generic

class Peer {
    public:
        /**
         * Creates & maintains a connection to a peer machine, enabling us to
         * send & receive messages of any format with this peer.
         * Messages are not guarenteed to reach the peer if the peer is down,
         * and will fail silently.
         *
         * @param listening_port - port we will listen for connections from this
         *      machine. NOTE: This port number must not be used by anything
         *      else because it is used to uniquely identify this peer.
         * @param destination_ip_address - ip address of the machine we want to
         *      peer with
         * @param destination_port - port the peer machine will be listening for
         *      our connection on. NOTE: Must not be shared on destination machine
         *      because it uniquely identifies our machine to the peer.
         * @param peer_message_received_callback - callback function to be called
         *      whenever we receive a message from this peer
         *      callback arguments:
         *          peer - who we received from
         *          char* - pointer to heap-allocated message data (client frees)
         *          int - size of message data
         */
        Peer(unsigned short listening_port, std::string destination_ip_address,
            unsigned short destination_port,
            std::function<void(Peer*, char*, int)> peer_message_received_callback);

        /**
         *  Destroy the Peer Connection & clean up all resources
         */
        ~Peer();

        /**
         * Attempts to send a message to a peer.  No guarentee that the message
         * will go through, client should handle resending if necessary.
         *
         * @param message - blob of data to send to the peer
         * @param message_len - size (in bytes) of message blob to send to peer
         */
        void SendMessage(const char* message, int message_len);

        /*
         * Identifier for this peer
         */
        int id;

    private:
        void AcceptConnection(const char* ip_addr, unsigned short port_num);
        void InitiateConnection(const char* ip_addr, unsigned short port_num);
        /**
         * Registers a listener that will repeatedly attempt to listen for
         * incoming connections on my_port coming from dest_ip_addr and updates
         * associated receiving state when establishing connection. If connection
         * is established, listens for messages coming in on the currently active
         * socket.  When messages are received, calls callback for every full
         * received chunk.
         */
        void RegisterReceiveListener();
        /**
         * Listens for broken outgoing connection (since we use two different
         * connections for send & receiving to minimize possible race conditions
         * at small cost & maintain implementation simplicity), and updates state
         * associated with outgoing connections on hearing the broken connection.
         */
        void RegisterCloseListener();
        void ListenOnSocket(int socket);

        // maybe TODO: in theory, we could reduce the number of connections
        // (currently seperate sockets for inbound & outbound connections),
        // introduces (solvable) race conditions
        // maybe TODO: similarly, could change to reuse listening port but would
        // need a "peer manager" that has knowledge of sockets, layered above
        // the peer (that hands-off connections to peers when it needs to send
        //a message to a particular peer, or gets a message & identifies which
        //peer sent it)

        /**
         * Explicitly track both sockets, even though we really only need one,
         * this is useful for debugging.
         *
         * NOTE: we could use a single socket for this, but then several
         * situations would become more complicated with potential for races
         * ("glare" situations where we try to connect at the same time
         * as the other server).
         *
         * Moreover, because raft only uses a small number of servers total to
         * get extremely negligible probability of failure
         * (P(failure) = probability down at any time ^ (num machines/2))
         * this won't end up using very many extra ports so seems worth
         * the reduced complexity
         */
        int send_socket;
        int receive_socket;
        unsigned short my_port;
        unsigned short dest_port;
        std::string dest_ip_addr;

        /**
         * Listening for incoming connection and/or incoming messages if
         * connection has already been established
         */
        std::thread in_listener;

        /**
         * Listening for the outgoing connection to break. Do not expect to
         * receive any real messages, assuming normal peering.
         */
        std::thread out_listener;

        /**
         * Indicates whether the connection has been reset, and therefore we
         * should join the old listening thread
         */
        bool connection_reset;
        /**
         * Indicates that the loops in the listening threads should continue
         * running. Changed to false when destructing.
         */
        bool running;










        // TODO: MERGED STREAMPARSER METHODS & VARIABLES (as per Ousterhout's suggestion)

        /**
         * Given a chunk of data from the stream of any size:
         *  - parses out any completed messages & calls message_received_callback
         *      that was passed in the constructor
         *  - accumulates any partial messages to be completed by future chunks.
         *
         * Must be called on bytes coming from stream in order.
         *
         * @param buffer - buffer containing received bytes from the stream
         * @param valid_bytes - number of valid bytes in this buffer from stream
         */
        void HandleRecievedChunk(char* buffer, int valid_bytes);

        /**
         * Resets/ throws out any partially accumulated message from the socket,
         * useful in cases where we know the message can never be completed
         */
        void ResetIncomingMessage();

        /**
         * Callback invoked every time we have accumulated a "complete" message
         * as defined by "was sent as CreateMessageToSend blob on other end of
         * stream".
         *
         * Callback arguments:
         *      - char* : the blob of bytes received
         *      - int : the length of the blob of bytes received
         */
        std::function<void(Peer*, char*, int)> message_received_callback;
        /*
         * How many bytes we have currently received as part of the next message
         * from this peer.
         */
        int current_message_length;
        /*
         * How many total bytes will be in our next message
         */
        int target_message_length;
        /*
         * Message under construction is a heap-allocated buffer that tracks
         * the "current" partially-received message that came over the
         * socket's stream. It will accumulate only until we have a full message.
         * This must be freed before you point to a new heap location
         */
        char* message_under_construction;

        /**
         * Because the stream of bytes may be cut even along within the middle
         * of the 4 bytes identifying the length of the subsequent message blob,
         * we may need to accumulate a partial integer.  We use this number &
         * buffer to accumulate bytes until the full number is complete.
         */
        /* number of bytes currently written in incomplete_number_buffer */
        int partial_number_bytes;
        /* accumulator for the leading integer, that precedes each message
         * and enables us to provide a message-based interface */
        char incomplete_number_buffer[sizeof(int)];
};
