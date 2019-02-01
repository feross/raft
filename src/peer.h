#pragma once

#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

#include "log.h"
#include "stream_parser.h"

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
         * @param message_received_callback - callback function to be called
         *      whenever we receive a message from this peer
         *      callback arguments:
         *          peer - who we received from
         *          char* - pointer to heap-allocated message data (client frees)
         *          int - size of message data
         */
        Peer(unsigned short listening_port, std::string destination_ip_address,
            unsigned short destination_port,
            std::function<void(Peer*, char*, int)> message_received_callback);

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

        /**
         * Class used to manage turning outgoing messages into a format that we
         * can easily parse on the other end (using this class), even though
         * we may receive it sliced up in any way.
         * 
         * Assumes stream bytes received in-order (though any slicing of bytes)
         */
        StreamParser *stream_parser;
};
