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
         *      machine.  This port number must not be shared with anything else
         *      because it uniquely identifies the peer.
         * @param destination_ip_address - ip address of the machine we want to
         *      peer with
         * @param destination_port - port the peer machine will be listening for
         *      our connection on.  Must not be shared on destination machine because
         *      it uniquely identifies our machine to the peer.
         * @param message_received_callback - callback function to be called
         *      whenever we receive a message
         *      callback arguments:
         *          peer - who we received from
         *          char* - pointer to heap-allocated message data
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
        void RegisterReceiveListener();
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

        int send_socket;
        int receive_socket;
        unsigned short my_port;
        unsigned short dest_port;
        std::string dest_ip_addr;
        std::thread in_listener;
        std::thread out_listener;
        bool connection_reset;
        bool running;
        StreamParser *stream_parser;
};
