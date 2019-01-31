#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include <iostream>
#include <string>

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <cassert>
// clean this up ^
#include "stream_parser.h"

#include "peer.h"

#define RECEIVE_BUFFER_SIZE 100000
#define DEBUG false

Peer::Peer(unsigned short listening_port, const char* destination_ip_address,
        unsigned short destination_port, std::function<void(Peer*, char*, int)> callback) {
    assert(listening_port != destination_port);

    my_port = listening_port;
    dest_port = destination_port;
    dest_ip_addr = destination_ip_address;
    connection_reset = false;

    send_socket = -1;

    if (DEBUG) printf("constructing Peer Instance\n");

    ListenForInboundMessages();

    stream_parser = new StreamParser([this, callback](char* raw_message, int raw_message_len) {
        callback(this, raw_message, raw_message_len);
    });
}


//feels like there is a way to get single-socket version working right, where either party can intiatiate a connection & the other side is always listening if it can
// (note: that would require separate threads, and therefore also protection over the one shared socket of some kind OR use lexicographical order to decide races consistently)


// overall, feels simpler to just separate the two flows & remove this sort of weird race condition case... although it also feels very slightly like it's
// passing complexity to the client, because the client now has to specify twice as many ports... but really seems worth it on simplifying implementation.

void Peer::ListenForInboundMessages() {
    if (DEBUG) printf("creating a new inbound listening thread\n");
    in_listener = std::thread([this] () {
        while(true) {
            int receive_socket = AcceptConnection(dest_ip_addr, my_port);
            if (DEBUG) printf("receive socket: %d\n", receive_socket);
            ReceiveMessages(receive_socket);
            close(receive_socket);
            stream_parser->ResetIncomingMessage(); //dump anything we haven't used
            sleep(1);
        }
    });
    // join in destructor
}


void Peer::ListenForClose() {
    if (DEBUG) printf("creating a new outbound-close listening thread\n");
    out_listener = std::thread([this] () {
        ReceiveMessages(send_socket);
        connection_reset = true;
        close(send_socket);
        send_socket = -1;
        if (DEBUG) printf("\nOutbound Connection Closed\n\n");
    });
}



// ^^^ shiould be able to re-use for listening on other socket too
void Peer::ReceiveMessages(int socket) {
    char buffer[RECEIVE_BUFFER_SIZE + 1]; /* +1 so we can add null terminator */
    while(socket > 0) {
        int len = recv(socket, buffer, RECEIVE_BUFFER_SIZE, 0);

        if (len == -1) {
            if (DEBUG) fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
            break;
        } else if (len == 0) {
            if (DEBUG) printf("\nPeer Disconnected\n\n");
            break;
        }
        stream_parser->HandleRecievedChunk(buffer, len);
        /* We have to null terminate the received data ourselves */  //NOTE: this is where we should be handling the protocol buffers stuff...
        //data won't normally come in this nice
        buffer[len] = '\0';
        if (DEBUG) printf("Received %s (%d bytes).\n", buffer, len);
    }
}


//user is not told whether succeeded or not, doesn't need to know
void Peer::SendMessage(const char* message, int message_len) {
    //might even want to do in thread, to return immediately to client of peer...
    if (send_socket == -1) {
        if (DEBUG) printf("attempted reconnection\n");
        if (connection_reset) {
            out_listener.join();
            connection_reset = false;
            if (DEBUG) printf("joining old thread\n");
        }
        InitiateConnection(dest_ip_addr, dest_port);
    }
    // technically, send_socket could get closed right here, but we don't care (send will just fail, and that's fine)
    if (send_socket > 0) {
        if (DEBUG) printf("send_socket x: %d\n", send_socket);

        char* formatted_message = stream_parser->CreateMessageToSend(message, message_len); //TODO change return type to include size somehow
        int success = send(send_socket, formatted_message, message_len+sizeof(int), 0); //See above TODO re: message_len+sizeof(int)
        if (success == -1) fprintf(stderr, "error send: %s (%d)\n", strerror(errno), errno);
        delete(formatted_message);
    }
    // user doesn't know if message got through, and that's fine
}
//unclear how to receive message, should maybe interrupt main line of execution... or at least, the main program needs to become aware of what this message was
//for now just have a thread that prints incoming, created in the constructor

int Peer::AcceptConnection(const char* ip_addr, int listening_port) {
    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             /* zero the struct before filling the fields */
    serv.sin_family = AF_INET;                  /* set the type of connection to TCP/IP */
    dest.sin_addr.s_addr = inet_addr(ip_addr);  /* set destination IP number - test with localhost, 127.0.0.1*/
    serv.sin_port = htons(listening_port);            /* set the server port number */

    mysocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mysocket == -1) fprintf(stderr, "socket error: %s (%d)\n", strerror(errno), errno);
    if (DEBUG) printf("listen_port: %d, mysocket: %d\n", listening_port, mysocket);

    int val = 1; //setsockopt why
    int success = setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
    if (success == -1) fprintf(stderr, "setsockopt error: %s (%d)\n", strerror(errno), errno);

    /* bind serv information to mysocket */
    success = bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));
    if (success == -1) fprintf(stderr, "bind error: %s (%d) socket: %d\n", strerror(errno), errno, mysocket);

    /* start listening, allowing a queue of up to 1 pending connection */
    success = listen(mysocket, 1);
    if (success == -1) fprintf(stderr, "listen error: %s (%d)\n", strerror(errno), errno);

    int connected_socket = accept(mysocket, (struct sockaddr *)&dest, &socksize);  //could just have one server here, and use the sockets returned by this to form peers
    if (connected_socket == -1) fprintf(stderr, "accept error: %s (%d)\n", strerror(errno), errno);

    success = close(mysocket);
    if (success == -1) fprintf(stderr, "close mysocket error: %s (%d)\n", strerror(errno), errno);

    if (DEBUG) printf("accepted connection, my: %d con: %d", mysocket, connected_socket);
    return connected_socket;
}


int Peer::InitiateConnection(const char* ip_addr, int destination_port) {
    struct sockaddr_in dest;

    send_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (send_socket == -1) fprintf(stderr, "socket error: %s (%d)\n", strerror(errno), errno);

    memset(&dest, 0, sizeof(dest));                 /* zero the struct */
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ip_addr);      /* set destination IP number - test with localhost, 127.0.0.1*/
    dest.sin_port = htons(destination_port);                /* set destination port number */
    int success = connect(send_socket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if (success == 0) {
        ListenForClose();
    } else {
        send_socket = -1;
        if (success == -1) fprintf(stderr, "connect error: %s (%d)\n", strerror(errno), errno);
    }
    return send_socket;
}
