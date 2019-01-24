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
// clean this up lol ^

#include "peer.h"

#define MAXRCVLEN 500


Peer::Peer(unsigned short source_port, const char* destination_ip_address, unsigned short destination_port) {
    my_port = source_port;
    dest_port = destination_port;
    dest_ip_addr = destination_ip_address;

    if (my_port < dest_port) {
        consocket = AttemptConnection(dest_ip_addr, dest_port);
    } else {
        consocket = AcceptConnection(dest_ip_addr, my_port);
    }
    printf("here\n");

    StartListener();
}


//feels like there is a way to get single-socket version working right, where either party can intiatiate a connection & the other side is always listening if it can
// (note: that would require separate threads necessarily, and therefore also protection over the one shared socket)


// overall, feels simpler to just separate the two flows & remove this sort of weird race condition case... although it also feels very slightly like it's
// passing complexity to the client, because the client now has to specify twice as many ports... but really seems worth it on simplifying implementation.

void Peer::StartListener() {
    listener = std::thread([this] () {
        char buffer[MAXRCVLEN + 1]; /* +1 so we can add null terminator */

        while(consocket) {
            int len = recv(consocket, buffer, MAXRCVLEN, 0);
            if (len == -1) {
                fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
                consocket = -1;
                break;
            } else if (len == 0) {
                //connection is closed?  Should this be the signal?
                printf("Peer Disconnected\n");
                consocket = -1;
                break;
            }
            /* We have to null terminate the received data ourselves */
            buffer[len] = '\0';
            printf("Received %s (%d bytes).\n", buffer, len);
        }
    });
}


//get socket if available
//if not available, create
// if create, also re-initialize listener thread (& join the old one)

//user is not told whether succeeded or not, doesn't need to know
void Peer::SendMessage(const char* message) {
    if (consocket == -1) {
        printf("attempted reconnection");
        listener.join();
        consocket = AttemptConnection(dest_ip_addr, dest_port);
        StartListener();
    }
    int success = send(consocket, message, strlen(message), 0);
}
//unclear how to recieve message, should maybe interrupt main line of execution... or at least, the main program needs to become aware of what this message was
//for now just have a thread that prints incoming, created in the constructor

int Peer::AcceptConnection(const char* ip_addr, int port_num) {
    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             /* zero the struct before filling the fields */
    serv.sin_family = AF_INET;                  /* set the type of connection to TCP/IP */
    dest.sin_addr.s_addr = inet_addr(ip_addr);  /* set destination IP number - test with localhost, 127.0.0.1*/ 
    serv.sin_port = htons(port_num);            /* set the server port number */    

    mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
    /* bind serv information to mysocket */
    bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr));

    /* start listening, allowing a queue of up to 1 pending connection */
    listen(mysocket, 1);
    int connected_socket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    printf("accepted connection, my: %d con: %d", mysocket, connected_socket);
    return connected_socket;
}

int Peer::AttemptConnection(const char* ip_addr, int port_num) {
    int mysocket;
    struct sockaddr_in dest; 

    mysocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&dest, 0, sizeof(dest));                 /* zero the struct */
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ip_addr);      /* set destination IP number - test with localhost, 127.0.0.1*/ 
    dest.sin_port = htons(port_num);                /* set destination port number */

    int success = connect(mysocket, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
    if (success == 0) return mysocket;
    else return -1;
}